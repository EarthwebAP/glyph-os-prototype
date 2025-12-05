# Runbook: Substrate Checksum Failures

**Alert**: `SubstrateChecksumFailures`
**Severity**: CRITICAL
**Component**: substrate_core

---

## Symptoms

- Alert: "Substrate checksum failures detected"
- Data integrity violations
- Possible memory or disk corruption

---

## Impact

**CRITICAL**: Data integrity compromise
- Field state may be corrupted
- Cell values unreliable
- Risk of catastrophic failure cascade

---

## Immediate Actions

### 1. STOP All Writes

```bash
# Disable glyph activations
service glyph_interpreter stop

# Put substrate in read-only mode (if supported)
kill -USR1 $(pgrep substrate_core)
```

### 2. Capture State

```bash
# Snapshot current state
mkdir -p /var/crash/glyphos-$(date +%Y%m%d-%H%M%S)
cd /var/crash/glyphos-$(date +%Y%m%d-%H%M%S)

# Dump substrate memory
gcore $(pgrep substrate_core)

# Copy vault
cp -a /var/db/glyphos/vault ./

# Copy logs
cp /var/log/glyphos/*.log ./

# Get metrics
curl http://localhost:9102/metrics > substrate_metrics.txt
```

### 3. Check Extent of Damage

```bash
# Run integrity check
/usr/local/bin/substrate_core --verify --vault /var/db/glyphos/vault

# Check parity status
grep -i parity /var/log/glyphos/substrate.log

# Count affected cells
grep -c "checksum" /var/log/glyphos/substrate.log
```

---

## Diagnosis

### Possible Causes

1. **Hardware failure**:
   - RAM bit flips (cosmic rays, failing DIMM)
   - Disk corruption
   - CPU cache errors

2. **Software bugs**:
   - Buffer overflows
   - Race conditions
   - Pointer corruption

3. **External interference**:
   - Power glitch
   - Kernel panic recovery
   - Malicious attack

### Investigation

**Check hardware**:
```bash
# RAM test (requires reboot)
memtest86+

# Disk SMART status
smartctl -a /dev/ada0

# CPU errors
dmesg | grep -i "machine check"

# ECC memory errors
dmesg | grep -i ecc
```

**Check software**:
```bash
# Core dumps
ls -l /var/crash/

# Sanitizer logs (if built with ASan)
grep -i "sanitizer" /var/log/glyphos/*.log

# Recent code changes
git log --since="1 week ago" -- src/substrate_core.c
```

---

## Resolution

### If Hardware Failure

**RAM issue**:
```bash
# Identify bad DIMM
memtest86+ (boot from USB)

# Replace hardware
# Restore from backup after replacement
```

**Disk issue**:
```bash
# Repair filesystem
fsck /dev/ada0p2

# If ZFS, check pool status
zpool status

# Restore from last known good backup
```

### If Software Bug

```bash
# Rollback to previous version
cd /usr/local/bin
mv substrate_core substrate_core.broken
cp substrate_core.backup substrate_core

# Restart service
service glyphd restart

# File bug report with crash dump
```

### Recovery Procedure

**1. Restore from backup**:
```bash
cd /var/backups/glyphos
# Find last known good backup (before checksum failures)
ls -lt | head -10

# Restore
tar -xzf glyphos-backup-20251204-020000.tar.gz -C /var/db/glyphos
chown -R glyphos:glyphos /var/db/glyphos
```

**2. Verify integrity**:
```bash
/usr/local/bin/substrate_core --verify --vault /var/db/glyphos/vault
```

**3. Restart services**:
```bash
service glyphd start
service glyph_interpreter start
```

**4. Monitor closely**:
```bash
# Watch for new checksum failures
tail -f /var/log/glyphos/substrate.log | grep -i checksum

# Monitor metrics
watch -n 5 'curl -s http://localhost:9102/metrics | grep checksum'
```

---

## Post-Incident

### 1. Root Cause Analysis

- Was it hardware or software?
- How many cells were affected?
- What was the data loss (if any)?
- Timeline of events

### 2. Prevention

**Hardware**:
- [ ] Enable ECC memory
- [ ] Monitor SMART status daily
- [ ] Increase backup frequency
- [ ] Consider redundant hardware

**Software**:
- [ ] Add more extensive parity checks
- [ ] Implement scrubbing daemon
- [ ] Add checksums to all critical data
- [ ] Enable ASan/MSan in staging

### 3. Update Monitoring

```yaml
# Add alert for early warning
- alert: ChecksumFailuresPredicted
  expr: rate(glyphos_substrate_cell_updates_total[1h]) > 10000 AND
        glyphos_substrate_avg_magnitude > 950
  for: 30m
  labels:
    severity: warning
```

---

## Escalation

**IMMEDIATE**:
- On-call engineer: Page immediately
- Database team: Within 5 minutes
- Security team: Within 10 minutes (if attack suspected)
- CTO: Within 15 minutes

**Severity: P0 (Critical)**

---

## Related Runbooks

- [Parity Failures](./parity-failures.md)
- [Disaster Recovery](./disaster-recovery.md)
- [Service Down](./service-down.md)
