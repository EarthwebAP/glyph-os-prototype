/*
 * GlyphOS Prometheus Metrics Implementation
 */

#include "metrics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

MetricsRegistry g_metrics = {0};

int metrics_init(void) {
    memset(&g_metrics, 0, sizeof(MetricsRegistry));
    pthread_mutex_init(&g_metrics.lock, NULL);
    return 0;
}

void metrics_destroy(void) {
    pthread_mutex_destroy(&g_metrics.lock);
}

/* Find or create metric */
static Metric* find_or_create_metric(const char* name, const char* help,
                                      MetricType type,
                                      MetricLabel* labels, int label_count) {
    pthread_mutex_lock(&g_metrics.lock);

    /* Search for existing metric */
    for (int i = 0; i < g_metrics.count; i++) {
        Metric* m = &g_metrics.metrics[i];
        if (strcmp(m->name, name) == 0 && m->type == type) {
            /* Check if labels match */
            if (m->label_count == label_count) {
                int match = 1;
                for (int j = 0; j < label_count; j++) {
                    if (strcmp(m->labels[j].key, labels[j].key) != 0 ||
                        strcmp(m->labels[j].value, labels[j].value) != 0) {
                        match = 0;
                        break;
                    }
                }
                if (match) {
                    pthread_mutex_unlock(&g_metrics.lock);
                    return m;
                }
            }
        }
    }

    /* Create new metric */
    if (g_metrics.count >= MAX_METRICS) {
        pthread_mutex_unlock(&g_metrics.lock);
        return NULL;
    }

    Metric* m = &g_metrics.metrics[g_metrics.count++];
    strncpy(m->name, name, sizeof(m->name) - 1);
    strncpy(m->help, help, sizeof(m->help) - 1);
    m->type = type;
    m->label_count = label_count;
    for (int i = 0; i < label_count && i < MAX_LABELS; i++) {
        m->labels[i] = labels[i];
    }
    gettimeofday(&m->last_updated, NULL);

    pthread_mutex_unlock(&g_metrics.lock);
    return m;
}

int metrics_counter_inc(const char* name, const char* help) {
    return metrics_counter_inc_by(name, help, 1.0);
}

int metrics_counter_inc_by(const char* name, const char* help, double value) {
    Metric* m = find_or_create_metric(name, help, METRIC_COUNTER, NULL, 0);
    if (!m) return -1;

    pthread_mutex_lock(&g_metrics.lock);
    m->value.counter_value += (unsigned long)value;
    gettimeofday(&m->last_updated, NULL);
    pthread_mutex_unlock(&g_metrics.lock);

    return 0;
}

int metrics_counter_inc_labels(const char* name, const char* help,
                                MetricLabel* labels, int label_count) {
    Metric* m = find_or_create_metric(name, help, METRIC_COUNTER, labels, label_count);
    if (!m) return -1;

    pthread_mutex_lock(&g_metrics.lock);
    m->value.counter_value++;
    gettimeofday(&m->last_updated, NULL);
    pthread_mutex_unlock(&g_metrics.lock);

    return 0;
}

int metrics_gauge_set(const char* name, const char* help, double value) {
    Metric* m = find_or_create_metric(name, help, METRIC_GAUGE, NULL, 0);
    if (!m) return -1;

    pthread_mutex_lock(&g_metrics.lock);
    m->value.gauge_value = value;
    gettimeofday(&m->last_updated, NULL);
    pthread_mutex_unlock(&g_metrics.lock);

    return 0;
}

int metrics_gauge_inc(const char* name, const char* help) {
    return metrics_gauge_add(name, help, 1.0);
}

int metrics_gauge_dec(const char* name, const char* help) {
    return metrics_gauge_add(name, help, -1.0);
}

int metrics_gauge_add(const char* name, const char* help, double delta) {
    Metric* m = find_or_create_metric(name, help, METRIC_GAUGE, NULL, 0);
    if (!m) return -1;

    pthread_mutex_lock(&g_metrics.lock);
    m->value.gauge_value += delta;
    gettimeofday(&m->last_updated, NULL);
    pthread_mutex_unlock(&g_metrics.lock);

    return 0;
}

int metrics_histogram_observe(const char* name, const char* help, double value,
                               double* buckets, int bucket_count) {
    Metric* m = find_or_create_metric(name, help, METRIC_HISTOGRAM, NULL, 0);
    if (!m) return -1;

    pthread_mutex_lock(&g_metrics.lock);

    /* Initialize buckets on first use */
    if (m->value.histogram.bucket_count == 0) {
        m->value.histogram.bucket_count = bucket_count;
        for (int i = 0; i < bucket_count && i < MAX_BUCKETS; i++) {
            m->value.histogram.buckets[i].le = buckets[i];
            m->value.histogram.buckets[i].count = 0;
        }
    }

    /* Update buckets */
    for (int i = 0; i < m->value.histogram.bucket_count; i++) {
        if (value <= m->value.histogram.buckets[i].le) {
            m->value.histogram.buckets[i].count++;
        }
    }

    m->value.histogram.count++;
    m->value.histogram.sum += value;
    gettimeofday(&m->last_updated, NULL);

    pthread_mutex_unlock(&g_metrics.lock);
    return 0;
}

char* metrics_export_prometheus(void) {
    static char buffer[65536];
    int offset = 0;

    pthread_mutex_lock(&g_metrics.lock);

    for (int i = 0; i < g_metrics.count; i++) {
        Metric* m = &g_metrics.metrics[i];

        /* Write HELP and TYPE */
        const char* type_str =
            m->type == METRIC_COUNTER ? "counter" :
            m->type == METRIC_GAUGE ? "gauge" :
            "histogram";

        offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                          "# HELP %s %s\n", m->name, m->help);
        offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                          "# TYPE %s %s\n", m->name, type_str);

        /* Write labels if present */
        char label_str[256] = "";
        if (m->label_count > 0) {
            int loff = 0;
            loff += snprintf(label_str + loff, sizeof(label_str) - loff, "{");
            for (int j = 0; j < m->label_count; j++) {
                if (j > 0) loff += snprintf(label_str + loff, sizeof(label_str) - loff, ",");
                loff += snprintf(label_str + loff, sizeof(label_str) - loff,
                                "%s=\"%s\"", m->labels[j].key, m->labels[j].value);
            }
            loff += snprintf(label_str + loff, sizeof(label_str) - loff, "}");
        }

        /* Write value */
        if (m->type == METRIC_COUNTER) {
            offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                              "%s%s %lu\n", m->name, label_str, m->value.counter_value);
        } else if (m->type == METRIC_GAUGE) {
            offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                              "%s%s %.6f\n", m->name, label_str, m->value.gauge_value);
        } else if (m->type == METRIC_HISTOGRAM) {
            /* Histogram buckets */
            for (int j = 0; j < m->value.histogram.bucket_count; j++) {
                offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                                  "%s_bucket{le=\"%.3f\"} %lu\n",
                                  m->name, m->value.histogram.buckets[j].le,
                                  m->value.histogram.buckets[j].count);
            }
            offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                              "%s_bucket{le=\"+Inf\"} %lu\n",
                              m->name, m->value.histogram.count);
            offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                              "%s_sum %.6f\n", m->name, m->value.histogram.sum);
            offset += snprintf(buffer + offset, sizeof(buffer) - offset,
                              "%s_count %lu\n", m->name, m->value.histogram.count);
        }

        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\n");
    }

    pthread_mutex_unlock(&g_metrics.lock);

    return buffer;
}
