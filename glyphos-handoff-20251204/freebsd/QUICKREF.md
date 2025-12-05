# GlyphOS Quick Reference Card

## Service Management

```bash
# Check status
service glyphd status
service glyph_spu status

# Start/stop/restart
service glyphd start|stop|restart
service glyph_spu start|stop|restart

# Enable/disable on boot
sysrc glyphd_enable="YES"
sysrc glyph_spu_enable="NO"
```

## Health Checks

```bash
# Services
curl http://localhost:8080/health  # glyphd
curl http://localhost:8081/health  # glyph-spu

# Metrics
curl http://localhost:9100/metrics  # node_exporter
curl http://localhost:9101/metrics  # glyphd_exporter
```

## Firewall (pf)

```bash
# Show rules
pfctl -sr

# Reload config
pfctl -f /etc/pf.conf

# Enable/disable
pfctl -e   # enable
pfctl -d   # disable

# Show blocked IPs
pfctl -t bruteforce -T show

# Clear blocked IPs
pfctl -t bruteforce -T flush
```

## Updates

```bash
# System update (base + packages + GlyphOS)
sudo glyphos-update

# Manual FreeBSD update
sudo freebsd-update fetch install

# Manual package update
sudo pkg update && sudo pkg upgrade
```

## ZFS Management

```bash
# Create snapshot
zfs snapshot glyphos/data@$(date +%Y%m%d-%H%M%S)

# List snapshots
zfs list -t snapshot

# Rollback
zfs rollback glyphos/data@20251205-120000

# Check pool status
zpool status
```

## Logs

```bash
# System logs
tail -f /var/log/messages

# Audit logs
praudit /var/audit/* | tail -50

# GlyphOS logs
tail -f /var/log/glyphos/glyphd.log
tail -f /var/log/glyphos/glyph-spu.log
```

## Network

```bash
# Show interfaces
ifconfig

# Show routing table
netstat -rn

# DHCP renewal
sudo dhclient em0

# Static IP (edit /etc/rc.conf)
ifconfig_em0="inet 192.168.1.100 netmask 255.255.255.0"
defaultrouter="192.168.1.1"
```

## SSH Access

```bash
# Add authorized key
mkdir -p ~/.ssh
echo "ssh-ed25519 AAAA..." >> ~/.ssh/authorized_keys
chmod 700 ~/.ssh
chmod 600 ~/.ssh/authorized_keys

# Connect
ssh -i ~/.ssh/id_ed25519 admin@glyphos-node
```

## Persistence

```bash
# Data directory
ls -la /usr/local/glyphos/data

# Check ownership
ls -ld /usr/local/glyphos/data
# Should show: drwx------  2 glyphd  glyphd

# Fix permissions
sudo chown -R glyphd:glyphd /usr/local/glyphos/data
sudo chmod 700 /usr/local/glyphos/data
```

## Troubleshooting

```bash
# Service won't start
tail /var/log/messages
/usr/local/bin/glyphd  # run manually

# Network issues
ping 8.8.8.8
drill google.com

# Disk space
df -h
du -sh /usr/local/glyphos/data

# Process list
ps aux | grep glyph

# Port listeners
sockstat -l -4
```

## Common Operations

### Create Glyph
```bash
curl -X POST http://localhost:8080/glyphs \
  -H "Content-Type: application/json" \
  -d '{"content": "test", "metadata": {}}'
```

### Query Glyph
```bash
curl http://localhost:8080/glyphs/<glyph_id>
```

### Merge Operation
```bash
curl -X POST http://localhost:8081/offload/merge \
  -H "Content-Type: application/json" \
  -d @merge_request.json
```

## Emergency

```bash
# Stop all GlyphOS services
service glyphd stop
service glyph_spu stop
service glyphd_exporter stop

# Disable firewall (temporary)
pfctl -d

# Emergency reboot
shutdown -r now

# Emergency halt
shutdown -p now
```

## Monitoring Alerts

| Alert | Check |
|-------|-------|
| Service down | `service glyphd status` |
| High load | `uptime`, `top` |
| Disk full | `df -h` |
| Network issues | `netstat -i`, `ifconfig` |
| SSH blocked | `pfctl -t bruteforce -T show` |

## Key File Locations

| Purpose | Path |
|---------|------|
| glyphd binary | /usr/local/bin/glyphd |
| glyph-spu binary | /usr/local/bin/glyph-spu |
| Persistence data | /usr/local/glyphos/data |
| Firewall config | /etc/pf.conf |
| SSH config | /etc/ssh/sshd_config |
| Boot config | /etc/rc.conf |
| Update script | /usr/local/sbin/glyphos-update |

---
**Version**: 1.0.0 | **Node**: `hostname`
