# GlyphOS Operationalization Guide

**Version**: 1.0
**Status**: 📋 Implementation Guide
**Last Updated**: 2025-12-05

---

## Overview

Comprehensive guide for deploying and operating GlyphOS in production, covering:
- Deployment procedures
- Monitoring and alerting
- Key rotation
- Backup and recovery
- Incident response
- On-call procedures

---

## Prerequisites

**Infrastructure**:
- [ ] FreeBSD 13.2+ server(s)
- [ ] ZFS storage configured
- [ ] Network connectivity (isolated VLAN recommended)
- [ ] Prometheus + Grafana deployed
- [ ] PagerDuty or equivalent alerting

**Access**:
- [ ] Non-root `glyphos` user created
- [ ] SSH key-based authentication
- [ ] sudo privileges for service management
- [ ] Firewall rules configured

**Software**:
- [ ] GlyphOS binaries built and verified
- [ ] All dependencies installed
- [ ] Monitoring infrastructure ready

---

## Initial Deployment

### 1. Server Preparation

```bash
# Create glyphos user
pw useradd glyphos -m -s /bin/sh -c "GlyphOS Service Account"

# Create directories
mkdir -p /var/db/glyphos/vault
mkdir -p /var/log/glyphos
mkdir -p /var/backups/glyphos

# Set ownership
chown -R glyphos:glyphos /var/db/glyphos
chown -R glyphos:glyphos /var/log/glyphos
```

### 2. Install Binaries

```bash
# Copy verified binaries
cp bin/substrate_core /usr/local/bin/
cp bin/glyph_interp /usr/local/bin/

# Verify checksums
sha256sum /usr/local/bin/substrate_core
# Compare with release manifest

# Set permissions
chmod 755 /usr/local/bin/substrate_core
chmod 755 /usr/local/bin/glyph_interp
```

### 3. Install Service Script

```bash
# Copy rc.d script
cp contrib/glyphd.rc /usr/local/etc/rc.d/glyphd
chmod 755 /usr/local/etc/rc.d/glyphd

# Enable service
sysrc glyphd_enable="YES"
sysrc glyphd_user="glyphos"
sysrc glyphd_vault="/var/db/glyphos/vault"
```

### 4. Configure Monitoring

```bash
# Verify metrics endpoints
curl http://localhost:9102/metrics  # substrate_core
curl http://localhost:9103/metrics  # glyph_interpreter

# Add to Prometheus scrape config
# (See docs/MONITORING.md)
```

### 5. Start Services

```bash
# Start GlyphOS
service glyphd start

# Verify
service glyphd status
ps aux | grep glyphos

# Check logs
tail -f /var/log/glyphos/substrate.log
```

---

## Key Rotation

### GPG Signing Keys

**Rotation Schedule**: Every 12 months

**Procedure**:

```bash
# 1. Generate new key
gpg --gen-key
# Email: glyphos-release@example.com
# Expiration: 2 years

# 2. Export public key
gpg --armor --export glyphos-release@example.com > glyphos-2026.pub

# 3. Update GitHub secret
gh secret set GPG_SIGNING_KEY < ~/.gnupg/private-key.asc

# 4. Revoke old key (after grace period)
gpg --gen-revoke OLD_KEY_ID > revoke.asc
gpg --import revoke.asc
gpg --send-keys OLD_KEY_ID
```

**Verification**:
```bash
# Test signing
echo "test" | gpg --clearsign -u glyphos-release@example.com

# Upload to CI and test
```

### Cosign Keyless Signing

**Rotation**: Automatic (uses ephemeral keys via Sigstore)

**Verification**:
```bash
# Verify artifact signature
cosign verify-blob \
  --signature substrate_core.sig \
  --certificate substrate_core.cert \
  substrate_core
```

### SSH Keys (Service Account)

**Rotation Schedule**: Every 6 months

```bash
# Generate new key
ssh-keygen -t ed25519 -f ~/.ssh/glyphos_2026 -C "glyphos@prod"

# Add to authorized_keys on servers
cat ~/.ssh/glyphos_2026.pub | ssh admin@server 'sudo -u glyphos tee -a ~/.ssh/authorized_keys'

# Update deployment scripts
# Grace period: 30 days overlap
# Remove old key after grace period
```

---

## Backup and Recovery

### Backup Schedule

**Frequency**:
- Vault data: Every 6 hours
- Logs: Daily
- Configuration: On change
- Full system: Weekly

**Retention**:
- Hourly: 7 days
- Daily: 30 days
- Weekly: 90 days
- Monthly: 1 year

### Automated Backup

**Cron configuration**:
```cron
# /etc/crontab or crontab -e -u glyphos

# Vault backup every 6 hours
0 */6 * * * /usr/local/bin/glyphos-backup.sh vault

# Log backup daily at 2 AM
0 2 * * * /usr/local/bin/glyphos-backup.sh logs

# Full backup weekly (Sunday 3 AM)
0 3 * * 0 /usr/local/bin/glyphos-backup.sh full
```

**Backup script** (`/usr/local/bin/glyphos-backup.sh`):
```bash
#!/bin/sh
set -e

TYPE="$1"
TIMESTAMP=$(date +%Y%m%d-%H%M%S)
BACKUP_DIR="/var/backups/glyphos"

case "$TYPE" in
  vault)
    tar -czf "$BACKUP_DIR/vault-$TIMESTAMP.tar.gz" \
      -C /var/db/glyphos vault
    ;;
  logs)
    tar -czf "$BACKUP_DIR/logs-$TIMESTAMP.tar.gz" \
      -C /var/log glyphos
    ;;
  full)
    tar -czf "$BACKUP_DIR/full-$TIMESTAMP.tar.gz" \
      -C /var/db glyphos \
      -C /var/log glyphos \
      -C /usr/local/etc rc.d/glyphd
    ;;
esac

# Verify backup
tar -tzf "$BACKUP_DIR/${TYPE}-$TIMESTAMP.tar.gz" > /dev/null

# Calculate checksum
sha256sum "$BACKUP_DIR/${TYPE}-$TIMESTAMP.tar.gz" \
  > "$BACKUP_DIR/${TYPE}-$TIMESTAMP.sha256"

echo "Backup complete: $BACKUP_DIR/${TYPE}-$TIMESTAMP.tar.gz"
```

### Off-site Backup

**Methods**:
- ZFS replication to secondary site
- rsync to backup server
- Cloud storage (S3, B2)

**Example**:
```bash
# rsync to backup server
rsync -avz --delete /var/backups/glyphos/ \
  backup-server:/backups/glyphos-prod/
```

### Recovery Procedure

**From backup**:
```bash
# Stop service
service glyphd stop

# Restore vault
cd /var/db/glyphos
rm -rf vault
tar -xzf /var/backups/glyphos/vault-TIMESTAMP.tar.gz

# Verify integrity
/usr/local/bin/substrate_core --verify --vault vault/

# Restart service
service glyphd start

# Verify operation
curl http://localhost:9102/metrics
```

**From ZFS snapshot**:
```bash
# List snapshots
zfs list -t snapshot

# Rollback to snapshot
zfs rollback tank/glyphos@backup-20251204

# Restart service
service glyphd restart
```

---

## Monitoring Operations

### Daily Health Checks

**Manual checks**:
```bash
# Service status
service glyphd status

# Metrics endpoint
curl -f http://localhost:9102/metrics

# Recent errors
tail -100 /var/log/glyphos/substrate.log | grep -i error

# Disk space
df -h /var/db/glyphos

# Memory usage
top -d 1 | grep glyphos
```

**Automated checks** (see `docs/MONITORING.md`):
- Prometheus scrapes every 15s
- Alerts evaluated every 30s
- PagerDuty notifications for critical alerts

### Weekly Review

**Checklist**:
- [ ] Review Grafana dashboards for trends
- [ ] Check for pending security updates
- [ ] Verify backup completion
- [ ] Review alert false positive rate
- [ ] Update runbooks if needed

### Monthly Maintenance

**Tasks**:
- [ ] Review and archive old logs
- [ ] Test disaster recovery procedure
- [ ] Rotate credentials (as scheduled)
- [ ] Update documentation
- [ ] Capacity planning review

---

## Incident Response

### On-Call Rotation

**Schedule**: 1-week rotations

**Primary on-call**:
- PagerDuty escalation: Immediate
- Response SLA: 15 minutes
- Responsibilities: Initial triage, mitigation

**Secondary on-call**:
- PagerDuty escalation: +15 minutes
- Backup for primary
- Subject matter expert

**Manager on-call**:
- PagerDuty escalation: +30 minutes (critical only)
- Executive decision authority
- External communication

### Incident Severity Levels

**P0 - Critical**:
- Service completely down
- Data corruption detected
- Security breach suspected
- **SLA**: 15-minute response, 1-hour mitigation

**P1 - High**:
- Service degraded
- High error rates
- Performance severely impacted
- **SLA**: 30-minute response, 4-hour resolution

**P2 - Medium**:
- Non-critical issues
- Minor performance degradation
- **SLA**: Next business day

**P3 - Low**:
- Cosmetic issues
- Feature requests
- **SLA**: Backlog

### Escalation Paths

```
Alert → PagerDuty → Primary On-Call
                         ↓ (15 min no response)
                    Secondary On-Call
                         ↓ (15 min no response)
                    Manager On-Call
                         ↓ (critical only)
                    CTO/VP Engineering
```

### Incident Communication

**Slack channels**:
- `#glyphos-incidents` - Active incident coordination
- `#glyphos-alerts` - Automated alerts (low noise)
- `#glyphos-general` - Status updates

**Status page**:
- Update within 15 minutes of incident start
- Provide ETA for resolution
- Post-mortem within 48 hours

---

## Runbook Index

All runbooks in `docs/runbooks/`:

**Critical**:
- [Service Down](./runbooks/service-down.md)
- [Checksum Failures](./runbooks/checksum-failures.md)
- [Parity Failures](./runbooks/parity-failures.md)

**Performance**:
- [High Latency](./runbooks/high-latency.md)
- [Memory Leak](./runbooks/memory-leak.md)

**Operational**:
- [Vault Corruption](./runbooks/vault-corruption.md)
- [Disk Full](./runbooks/disk-full.md)
- [Disaster Recovery](./runbooks/disaster-recovery.md)

---

## Change Management

### Standard Change

**Process**:
1. Create pull request
2. Code review (2 approvals required)
3. CI checks pass
4. Deploy to staging
5. Soak test (24 hours minimum)
6. Deploy to production (canary → full)

### Emergency Change

**When**: P0 incident requires immediate fix

**Process**:
1. Create hotfix branch
2. Minimal fix only
3. Emergency review (2 senior engineers)
4. Deploy with monitoring
5. Post-incident review required

---

## Capacity Planning

### Metrics to Monitor

- CPU utilization
- Memory usage
- Disk I/O
- Network bandwidth
- Vault size growth
- Activation rate trends

### Scaling Triggers

**Vertical scaling** (upgrade server):
- CPU sustained > 70%
- Memory sustained > 80%
- Disk I/O saturated

**Horizontal scaling** (add servers):
- Not yet supported (single-instance design)
- Future: Distributed substrate with consensus

---

## Compliance and Auditing

### Audit Log

**What to log**:
- All vault modifications
- Glyph activations
- Administrative actions
- Configuration changes
- Security events

**Retention**: 1 year minimum

**Format**: Structured JSON to syslog

### Security Scanning

**Schedule**:
- Weekly: Dependency scans (Dependabot)
- Monthly: Container image scans
- Quarterly: Penetration testing
- Annually: Full security audit

---

## Training and Documentation

### New Engineer Onboarding

**Day 1**:
- Read architecture documentation
- Set up local development environment
- Run test suite locally

**Week 1**:
- Shadow on-call engineer
- Review all runbooks
- Practice incident scenarios

**Week 2**:
- Secondary on-call
- Handle non-critical incidents
- Update documentation

**Week 4**:
- Primary on-call rotation
- Full incident response authority

### Documentation Requirements

**All runbooks must include**:
- Symptoms
- Impact assessment
- Diagnosis steps
- Resolution procedure
- Escalation path
- Prevention measures

---

## Success Metrics

**Operational KPIs**:
- Uptime: 99.9% target
- Mean time to detect (MTTD): < 5 minutes
- Mean time to resolve (MTTR): < 1 hour (P0)
- Backup success rate: 100%
- False positive alert rate: < 5%

---

## References

- [Monitoring Guide](./MONITORING.md)
- [Security Patches](./SECURITY_PATCHES.md)
- [Implementation Roadmap](./IMPLEMENTATION_ROADMAP.md)
- [CI Improvements](./CI_IMPROVEMENTS.md)
