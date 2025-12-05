# Runbook: GlyphOS Service Down

**Alert**: `GlyphOSServiceDown`
**Severity**: CRITICAL
**Component**: substrate_core or glyph_interpreter

---

## Symptoms

- Prometheus `/metrics` endpoint not responding
- Service health check failing
- Alert: "GlyphOS service [substrate|interpreter] is down"

---

## Impact

**substrate_core down**:
- All field state operations halted
- No glyph activations possible
- Data loss risk if writes in progress

**glyph_interpreter down**:
- No new glyph activations
- Vault modifications not processed
- Existing substrate state preserved

---

## Diagnosis

### 1. Check Service Status

```bash
# FreeBSD
service glyphd status

# Check if process is running
ps aux | grep -E 'substrate_core|glyph_interp'

# Check metrics endpoint
curl -f http://localhost:9102/metrics  # substrate_core
curl -f http://localhost:9103/metrics  # glyph_interpreter
```

### 2. Check Logs

```bash
# Service logs
tail -100 /var/log/glyphos/substrate.log
tail -100 /var/log/glyphos/interpreter.log

# System logs
tail -100 /var/log/messages | grep glyphos

# Check for crashes
ls -l /var/crash/
```

### 3. Check Resources

```bash
# Memory
top -d 1

# Disk space
df -h /var/db/glyphos

# File descriptors
lsof -p $(pgrep substrate_core)
```

---

## Resolution

### Quick Fix (Restart)

```bash
# Restart service
service glyphd restart

# Verify metrics endpoint
curl http://localhost:9102/metrics

# Check logs
tail -f /var/log/glyphos/substrate.log
```

### If Restart Fails

**Check port conflicts**:
```bash
sockstat -4 -l | grep -E '9102|9103'
```

**Check permissions**:
```bash
ls -la /var/db/glyphos/vault
sudo -u glyphos /usr/local/bin/substrate_core --test
```

**Check vault integrity**:
```bash
cd /var/db/glyphos
find vault -name "*.gdf" -type f | wc -l
md5 vault/*.gdf
```

### If Crash Loop

```bash
# Run in foreground to see errors
sudo -u glyphos /usr/local/bin/substrate_core --test

# Check for corruption
/usr/local/bin/substrate_core --vault /var/db/glyphos/vault --verify

# Restore from backup if needed
cd /var/backups/glyphos
ls -lt | head -5
tar -xzf glyphos-backup-YYYYMMDD.tar.gz -C /var/db/glyphos
```

---

## Post-Incident

### 1. Update Status Page

```
Service: [substrate_core|glyph_interpreter]
Status: Resolved
Duration: [X minutes]
Root Cause: [Brief description]
```

### 2. Document Root Cause

- What triggered the outage?
- Why didn't monitoring catch it earlier?
- What prevented auto-recovery?

### 3. Prevention

- Add monitoring for root cause
- Improve health checks
- Add circuit breakers if applicable
- Update this runbook

---

## Escalation

**Severity: Critical**
- On-call engineer: Immediately
- Team lead: Within 15 minutes
- Engineering manager: Within 30 minutes

**Contact**:
- PagerDuty: glyphos-oncall
- Slack: #glyphos-incidents
- Email: oncall@glyphos.local

---

## Related Runbooks

- [High Latency](./high-latency.md)
- [Checksum Failures](./checksum-failures.md)
- [Disaster Recovery](./disaster-recovery.md)
