/*
 * Simple HTTP server for Prometheus metrics endpoint
 * Minimal implementation - production should use a proper HTTP library
 */

#include "metrics_server.h"
#include "metrics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

static int server_fd = -1;
static pthread_t server_thread;
static int server_running = 0;

static void* metrics_server_thread(void* arg) {
    int port = *(int*)arg;
    struct sockaddr_in addr;
    int opt = 1;

    /* Create socket */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        fprintf(stderr, "metrics_server: socket() failed\n");
        return NULL;
    }

    /* Allow address reuse */
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* Bind to port */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "metrics_server: bind() failed on port %d\n", port);
        close(server_fd);
        return NULL;
    }

    /* Listen */
    if (listen(server_fd, 5) < 0) {
        fprintf(stderr, "metrics_server: listen() failed\n");
        close(server_fd);
        return NULL;
    }

    printf("Metrics server listening on port %d\n", port);

    /* Accept connections */
    while (server_running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (server_running) {
                fprintf(stderr, "metrics_server: accept() failed\n");
            }
            break;
        }

        /* Read HTTP request (minimal parsing) */
        char req_buf[1024];
        ssize_t n = read(client_fd, req_buf, sizeof(req_buf) - 1);
        if (n > 0) {
            req_buf[n] = '\0';

            /* Check if it's a GET /metrics request */
            if (strstr(req_buf, "GET /metrics") != NULL) {
                /* Export metrics */
                char* metrics = metrics_export_prometheus();

                /* Send HTTP response */
                const char* http_header =
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/plain; version=0.0.4\r\n"
                    "Connection: close\r\n"
                    "\r\n";

                write(client_fd, http_header, strlen(http_header));
                write(client_fd, metrics, strlen(metrics));
            } else {
                /* 404 for other paths */
                const char* not_found =
                    "HTTP/1.1 404 Not Found\r\n"
                    "Content-Type: text/plain\r\n"
                    "Connection: close\r\n"
                    "\r\n"
                    "404 Not Found\n"
                    "Try GET /metrics\n";
                write(client_fd, not_found, strlen(not_found));
            }
        }

        close(client_fd);
    }

    close(server_fd);
    return NULL;
}

int metrics_server_start(int port) {
    if (server_running) {
        return -1;
    }

    server_running = 1;
    static int server_port;
    server_port = port;

    if (pthread_create(&server_thread, NULL, metrics_server_thread, &server_port) != 0) {
        server_running = 0;
        return -1;
    }

    return 0;
}

void metrics_server_stop(void) {
    if (!server_running) {
        return;
    }

    server_running = 0;

    /* Close server socket to unblock accept() */
    if (server_fd >= 0) {
        close(server_fd);
        server_fd = -1;
    }

    pthread_join(server_thread, NULL);
}
