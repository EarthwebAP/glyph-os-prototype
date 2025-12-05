# GlyphOS Monitoring Guide

**Version**: 1.0
**Status**: ✅ Implemented
**Last Updated**: 2025-12-05

---

## Overview

GlyphOS includes comprehensive monitoring infrastructure based on Prometheus and Grafana, providing:
- Real-time metrics collection
- Production dashboards
- Automated alerting
- Incident response runbooks

---

## Architecture

```
┌─────────────────┐     :9102      ┌────────────────┐
│ substrate_core  │────────────────▶│  Prometheus    │
└─────────────────┘                 │                │
                                    │  - Metrics     │
┌─────────────────┐     :9103      │  - Alerts      │      ┌─────────────┐
│ glyph_interp    │────────────────▶│  - Rules       │─────▶│   Grafana   │
└─────────────────┘                 │                │      │             │
                                    │  Port :9090    │      │  Port :3000 │
┌─────────────────┐     :9101      └────────────────┘      └─────────────┘
│ glyphd_exporter │────────────────▶                                │
└─────────────────┘                                                 │
                                    ┌────────────────┐              │
                                    │  AlertManager  │◀─────────────┘
                                    │                │
                                    │  Port :9093    │
                                    └────────────────┘
                                            │
                                            ▼
                                    ┌────────────────┐
                                    │   PagerDuty    │
                                    │   Slack, etc.  │
                                    └────────────────┘
```

---

## Metrics Endpoints

### Substrate Core (:9102/metrics)

**Cell Operations**:
- `glyphos_substrate_cells_written_total` - Total cells written (counter)
- `glyphos_substrate_cells_read_total` - Total cells read (counter)
- `glyphos_substrate_checksum_failures_total` - Checksum failures (counter)
- `glyphos_substrate_parity_failures_total` - Parity failures (counter)

**Performance**:
- `glyphos_substrate_update_duration_seconds` - Update latency histogram
  - Buckets: [0.001, 0.005, 0.01, 0.05, 0.1, 0.5, 1.0, 5.0]

**Field State**:
- `glyphos_substrate_avg_magnitude` - Average field magnitude (gauge)
- `glyphos_substrate_avg_coherence` - Average coherence (gauge)

### Glyph Interpreter (:9103/metrics)

**Vault Management**:
- `glyphos_vault_glyphs_total` - Total glyphs in vault (gauge)

**Activations**:
- `glyphos_glyph_activations_total{glyph_id}` - Activations per glyph (counter)
- `glyphos_glyph_activation_duration_seconds{glyph_id}` - Activation latency histogram
- `glyphos_glyph_activation_failures_total{reason}` - Failures by reason (counter)

**Parsing**:
- `glyphos_gdf_parse_errors_total{field}` - Parse errors by field (counter)

---

## Dashboards

### 1. Production Overview

**File**: `monitoring/grafana/production-overview.json`

**Panels**:
- Service status (substrate + interpreter uptime)
- Total glyph activations (rate)
- Substrate cell operations (reads/writes)
- Activation latency (p95)
- Error rates (failures + checksum errors)
- Field magnitude gauge
- Coherence gauge

**Use**: Primary production monitoring dashboard

**Refresh**: 10 seconds

### 2. Substrate Deep Dive

**Panels** (planned):
- Cell distribution heatmap
- Wave propagation visualization
- Checksum failure locations
- Memory usage breakdown
- Parity bit status

### 3. Performance Analysis

**Panels** (planned):
- Latency percentiles (p50, p95, p99)
- Throughput trends
- Resource utilization
- Queue depths
- Cache hit rates

### 4. Security Monitoring

**Panels** (planned):
- Vault access patterns
- Failed authentication attempts
- Path traversal attempts blocked
- Circular inheritance detections
- Audit log summary

---

## Alert Rules

### Critical Alerts (Immediate Response)

#### GlyphOSServiceDown
- **Condition**: `up{job=~"glyphos-.*"} == 0`
- **Duration**: 1 minute
- **Severity**: CRITICAL
- **Runbook**: [docs/runbooks/service-down.md](./runbooks/service-down.md)

#### HighActivationFailureRate
- **Condition**: `rate(glyphos_glyph_activation_failures_total[5m]) > 0.1`
- **Duration**: 2 minutes
- **Severity**: CRITICAL
- **Runbook**: [docs/runbooks/activation-failures.md](./runbooks/activation-failures.md)

#### SubstrateChecksumFailures
- **Condition**: `rate(glyphos_substrate_checksum_failures_total[5m]) > 0`
- **Duration**: 1 minute
- **Severity**: CRITICAL
- **Runbook**: [docs/runbooks/checksum-failures.md](./runbooks/checksum-failures.md)

#### SubstrateParityFailures
- **Condition**: `rate(glyphos_substrate_parity_failures_total[5m]) > 0`
- **Duration**: 1 minute
- **Severity**: CRITICAL
- **Runbook**: [docs/runbooks/parity-failures.md](./runbooks/parity-failures.md)

### Warning Alerts (Investigate)

#### HighActivationLatency
- **Condition**: `histogram_quantile(0.95, rate(...)) > 1.0`
- **Duration**: 5 minutes
- **Severity**: WARNING

#### FieldMagnitudeNearLimit
- **Condition**: `glyphos_substrate_avg_magnitude > 900`
- **Duration**: 10 minutes
- **Severity**: WARNING

#### LowCoherence
- **Condition**: `glyphos_substrate_avg_coherence < 100`
- **Duration**: 10 minutes
- **Severity**: WARNING

### Info Alerts (Informational)

#### VaultGlyphCountChanged
- **Condition**: `abs(delta(glyphos_vault_glyphs_total[5m])) > 0`
- **Severity**: INFO

---

## Setup Instructions

### 1. Install Dependencies

```bash
# FreeBSD
pkg install prometheus grafana node_exporter

# Enable services
sysrc prometheus_enable="YES"
sysrc grafana_enable="YES"
sysrc node_exporter_enable="YES"
```

### 2. Configure Prometheus

```bash
# Copy configuration
cp monitoring/prometheus/prometheus.yml /usr/local/etc/prometheus/
cp monitoring/prometheus/alerts.yml /usr/local/etc/prometheus/

# Validate configuration
promtool check config /usr/local/etc/prometheus/prometheus.yml
promtool check rules /usr/local/etc/prometheus/alerts.yml

# Restart Prometheus
service prometheus restart
```

### 3. Configure Grafana

```bash
# Start Grafana
service grafana start

# Access UI: http://localhost:3000
# Default credentials: admin/admin

# Add Prometheus data source
# - URL: http://localhost:9090
# - Access: Server (default)

# Import dashboard
# - Upload monitoring/grafana/production-overview.json
```

### 4. Start GlyphOS with Metrics

```bash
# substrate_core with metrics on :9102
/usr/local/bin/substrate_core --metrics-port 9102 &

# glyph_interpreter with metrics on :9103
/usr/local/bin/glyph_interp --metrics-port 9103 &

# Verify endpoints
curl http://localhost:9102/metrics
curl http://localhost:9103/metrics
```

---

## Monitoring Best Practices

### 1. Alert Fatigue Prevention

- **Tune thresholds** based on baseline
- **Use appropriate durations** to avoid flapping
- **Severity levels**:
  - CRITICAL: Page on-call immediately
  - WARNING: Create ticket, investigate next day
  - INFO: Logging only, no action

### 2. Runbook Quality

Every alert must have:
- ✅ Symptoms (what's wrong)
- ✅ Impact (business/user impact)
- ✅ Diagnosis steps
- ✅ Resolution steps
- ✅ Escalation path

### 3. Dashboard Organization

- **Overview dashboards**: High-level health
- **Component dashboards**: Deep-dive per service
- **On-call dashboards**: Everything needed for 3 AM pages

### 4. Metric Naming

Follow Prometheus conventions:
- `<namespace>_<subsystem>_<name>_<unit>`
- Example: `glyphos_substrate_cells_written_total`
- Units: `_seconds`, `_bytes`, `_total`, `_ratio`

---

## Troubleshooting

### Metrics Not Appearing

```bash
# Check service is running
curl -f http://localhost:9102/metrics

# Check Prometheus targets
curl http://localhost:9090/api/v1/targets | jq

# Check Prometheus logs
tail -f /var/log/prometheus/prometheus.log

# Check firewall
sockstat -4 -l | grep -E '9102|9103'
```

### Grafana Can't Connect to Prometheus

```bash
# Test connection
curl -f http://localhost:9090/api/v1/query?query=up

# Check Grafana logs
tail -f /var/log/grafana/grafana.log

# Verify data source configuration in Grafana UI
```

### Alerts Not Firing

```bash
# Check alert rules loaded
curl http://localhost:9090/api/v1/rules | jq

# Manually query alert condition
curl 'http://localhost:9090/api/v1/query?query=up{job="glyphos-substrate"}'

# Check AlertManager
curl http://localhost:9093/api/v1/alerts
```

---

## Development

### Adding New Metrics

**In C code**:
```c
#include "metrics.h"

// Initialize metrics library
metrics_init();

// Start HTTP server
metrics_server_start(9102);

// Increment counter
metrics_counter_inc("glyphos_operations_total", "Total operations");

// Set gauge
metrics_gauge_set("glyphos_queue_depth", "Queue depth", queue_size);

// Record histogram
double latency_buckets[] = {0.001, 0.01, 0.1, 1.0};
metrics_histogram_observe("glyphos_latency_seconds", "Latency",
                          elapsed, latency_buckets, 4);
```

### Testing Metrics Locally

```bash
# Build with metrics support
cc -o bin/substrate_core src/substrate_core.c src/metrics.c \
   src/metrics_server.c -lm -lpthread

# Run and check endpoint
./bin/substrate_core --metrics-port 9102 &
curl http://localhost:9102/metrics

# Should see Prometheus format output
```

---

## Production Checklist

Before deploying monitoring:

- [ ] Prometheus configured and running
- [ ] Grafana configured with data source
- [ ] All dashboards imported and tested
- [ ] Alert rules loaded and validated
- [ ] AlertManager integrated with PagerDuty
- [ ] Runbooks reviewed and accessible
- [ ] On-call rotation documented
- [ ] Escalation procedures defined
- [ ] Metrics retention configured (90 days recommended)
- [ ] Backup of Prometheus data enabled

---

## Metrics Retention

**Prometheus default**: 15 days

**Recommended**:
```yaml
# In prometheus.yml
storage:
  tsdb:
    retention.time: 90d
    retention.size: 50GB
```

**Long-term storage**: Consider Thanos or Cortex for >90 days

---

## Cost Estimation

**For single production instance**:

| Component | Resources | Cost/Month |
|-----------|-----------|------------|
| Prometheus | 4GB RAM, 50GB disk | $20 |
| Grafana | 2GB RAM, 10GB disk | $10 |
| AlertManager | 1GB RAM | $5 |
| **Total** | | **$35/month** |

---

## References

- **Prometheus docs**: https://prometheus.io/docs/
- **Grafana docs**: https://grafana.com/docs/
- **Alert rules**: `monitoring/prometheus/alerts.yml`
- **Dashboards**: `monitoring/grafana/*.json`
- **Runbooks**: `docs/runbooks/`

---

## Support

- **Issues**: https://github.com/EarthwebAP/glyph-os-prototype/issues
- **On-call**: PagerDuty glyphos-oncall
- **Slack**: #glyphos-monitoring
