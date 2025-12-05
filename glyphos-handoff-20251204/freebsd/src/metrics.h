/*
 * GlyphOS Prometheus Metrics Library
 * Lightweight metrics collection for monitoring
 */

#ifndef METRICS_H
#define METRICS_H

#include <time.h>
#include <sys/time.h>
#include <pthread.h>

/* Maximum number of metric series */
#define MAX_METRICS 128
#define MAX_LABELS 8
#define MAX_BUCKETS 16

/* Metric types */
typedef enum {
    METRIC_COUNTER,
    METRIC_GAUGE,
    METRIC_HISTOGRAM
} MetricType;

/* Label key-value pair */
typedef struct {
    char key[64];
    char value[64];
} MetricLabel;

/* Histogram bucket */
typedef struct {
    double le;        /* Upper bound (less than or equal) */
    unsigned long count;
} HistogramBucket;

/* Individual metric */
typedef struct {
    char name[128];
    char help[256];
    MetricType type;

    /* Labels */
    MetricLabel labels[MAX_LABELS];
    int label_count;

    /* Values */
    union {
        double gauge_value;
        unsigned long counter_value;
        struct {
            HistogramBucket buckets[MAX_BUCKETS];
            int bucket_count;
            unsigned long count;
            double sum;
        } histogram;
    } value;

    /* Timestamp */
    struct timeval last_updated;
} Metric;

/* Metrics registry */
typedef struct {
    Metric metrics[MAX_METRICS];
    int count;
    pthread_mutex_t lock;
} MetricsRegistry;

/* Global registry */
extern MetricsRegistry g_metrics;

/* Initialization */
int metrics_init(void);
void metrics_destroy(void);

/* Counter operations */
int metrics_counter_inc(const char* name, const char* help);
int metrics_counter_inc_by(const char* name, const char* help, double value);
int metrics_counter_inc_labels(const char* name, const char* help,
                                MetricLabel* labels, int label_count);

/* Gauge operations */
int metrics_gauge_set(const char* name, const char* help, double value);
int metrics_gauge_inc(const char* name, const char* help);
int metrics_gauge_dec(const char* name, const char* help);
int metrics_gauge_add(const char* name, const char* help, double delta);

/* Histogram operations */
int metrics_histogram_observe(const char* name, const char* help, double value,
                               double* buckets, int bucket_count);

/* Export */
char* metrics_export_prometheus(void);

/* Utility: Get current time in seconds */
static inline double metrics_now(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

/* Utility: Duration measurement */
typedef struct {
    struct timeval start;
} Timer;

static inline void timer_start(Timer* t) {
    gettimeofday(&t->start, NULL);
}

static inline double timer_elapsed(Timer* t) {
    struct timeval now;
    gettimeofday(&now, NULL);
    return (now.tv_sec - t->start.tv_sec) +
           (now.tv_usec - t->start.tv_usec) / 1000000.0;
}

#endif /* METRICS_H */
