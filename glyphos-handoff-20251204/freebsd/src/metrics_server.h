/*
 * Simple HTTP server for Prometheus metrics
 */

#ifndef METRICS_SERVER_H
#define METRICS_SERVER_H

/* Start HTTP server on given port */
int metrics_server_start(int port);

/* Stop HTTP server */
void metrics_server_stop(void);

#endif /* METRICS_SERVER_H */
