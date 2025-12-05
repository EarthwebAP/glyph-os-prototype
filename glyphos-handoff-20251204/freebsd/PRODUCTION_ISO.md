# GlyphOS Production ISO - Complete Guide

**Version:** 1.0.0  
**Date:** 2025-12-05  
**Build Script:** `build_iso.sh`

## Overview

The production ISO build includes all 6 required enhancements for deploying production-ready GlyphOS nodes:

1. ✅ **Package Management** - pkg bootstrap, repositories, initial package set
2. ✅ **Network Configuration** - DHCP/static IP, DNS, SSH hardening
3. ✅ **Security Hardening** - pf firewall, sysctl, auditd, securelevel
4. ✅ **Monitoring** - node_exporter, glyphd_exporter, health checks
5. ✅ **Persistence Layer** - ZFS/UFS writable partition for glyph storage
6. ✅ **Update Mechanism** - freebsd-update, package updates, patching

## Prerequisites

### Build Host Requirements

- FreeBSD 14.0 or later
- Root or sudo access
- Internet connection (for downloading FreeBSD base)
- Disk space: ~2GB for build artifacts
- Build tools: `pkg install cdrtools` or `pkg install xorriso`

### Runtime Requirements

Before building, compile the Rust binaries:

```bash
cd ../runtime/rust
cargo build --release --workspace
```

This creates:
- `target/release/glyphd` (node daemon)
- `target/release/glyph-spu` (SPU offload service)

## Building the ISO

```bash
cd /home/daveswo/glyph-os-prototype/glyphos-handoff-20251204/freebsd
sudo ./build_iso.sh
```

### Build Process (17 Steps)

The script performs these operations:

1. **Fetch FreeBSD Base** - Downloads base.txz and kernel.txz from official mirrors
2. **Extract Base** - Unpacks into ISO work directory
3. **Setup pkg** - Creates /usr/local/etc/pkg/repos/FreeBSD.conf, bootstraps pkg
4. **Copy Overlays** - Applies rc.d scripts and configuration
5. **Install Rust Binaries** - Copies glyphd and glyph-spu
6. **Network Config** - Creates rc.conf and resolv.conf
7. **SSH Hardening** - Configures sshd_config with modern ciphers
8. **pf Firewall** - Creates pf.conf with default-deny policy
9. **sysctl Hardening** - Network and kernel security settings
10. **Auditd** - Configures system auditing
11. **Monitoring** - Installs node_exporter, creates glyphd_exporter
12. **Persistence** - Sets up ZFS/UFS data directories
13. **Updates** - Creates glyphos-update script and freebsd-update.conf
14. **Users** - Creates glyphd service account
15. **Permissions** - Secures configuration files
16. **Boot Config** - Creates loader.conf
17. **Create ISO** - Builds bootable ISO with mkisofs/xorriso

### Build Artifacts

- **ISO File**: `glyphos-freebsd-0.1.0.iso` (in current directory)
- **Work Directory**: `/tmp/glyphos-build` (intermediate files)
- **Size**: ~800MB compressed, ~2GB extracted

## Enhancement Details

### 1. Package Management

**Location**: `/usr/local/etc/pkg/repos/FreeBSD.conf`

Configures FreeBSD official package repository:
```
FreeBSD: {
  url: "pkg+http://pkg.FreeBSD.org/${ABI}/quarterly",
  signature_type: "fingerprints",
  fingerprints: "/usr/share/keys/pkg"
}
```

**Pre-installed Packages**:
- `rust` - Rust toolchain
- `wasmtime` - WebAssembly runtime
- `node_exporter` - Prometheus system metrics
- `bash`, `curl`, `vim-console`, `tmux` - System utilities

**Usage**:
```bash
# Update package database
pkg update

# Install additional packages
pkg install postgresql15-server

# Upgrade all packages
pkg upgrade
```

### 2. Network Configuration

**Location**: `/etc/rc.conf`, `/etc/resolv.conf`

**DHCP (Default)**:
```
ifconfig_DEFAULT="DHCP"
```

**Static IP Template**:
```bash
# Edit /etc/rc.conf
ifconfig_em0="inet 192.168.1.100 netmask 255.255.255.0"
defaultrouter="192.168.1.1"
```

**DNS Servers**:
- Primary: 8.8.8.8 (Google)
- Secondary: 8.8.4.4 (Google)
- Tertiary: 1.1.1.1 (Cloudflare)

### 3. Security Hardening

#### pf Firewall

**Location**: `/etc/pf.conf`

Default-deny policy with explicit allows:
- SSH (port 22) - rate limited (3 attempts/30s)
- glyphd (port 8080) - state tracking
- glyph-spu (port 8081) - state tracking
- node_exporter (port 9100) - metrics

**SSH Brute-force Protection**:
```
max-src-conn 5
max-src-conn-rate 3/30
overload <bruteforce> flush global
```

**Managing Firewall**:
```bash
# Reload rules
pfctl -f /etc/pf.conf

# Show current rules
pfctl -sr

# Show blocked IPs
pfctl -t bruteforce -T show

# Clear blocked IPs
pfctl -t bruteforce -T flush
```

#### SSH Hardening

**Location**: `/etc/ssh/sshd_config`

Security features:
- Root login disabled
- Password authentication disabled (key-only)
- Modern ciphers only (ChaCha20-Poly1305, AES-GCM)
- Ed25519 host keys
- Rate limiting (3 attempts, 30s grace period)

**Adding SSH Keys**:
```bash
# On the ISO-booted system
mkdir -p /home/admin/.ssh
cat > /home/admin/.ssh/authorized_keys << 'EOF'
ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIB... admin@workstation
EOF
chmod 700 /home/admin/.ssh
chmod 600 /home/admin/.ssh/authorized_keys
chown -R admin:admin /home/admin/.ssh
```

#### sysctl Hardening

**Location**: `/etc/sysctl.conf`

- IP forwarding disabled
- ICMP redirects disabled
- Source routing disabled
- SYN cookie protection (SYN flood mitigation)
- TCP/UDP blackhole (stealth mode)
- Securelevel 1 (kernel immutability)
- Process isolation (see_other_uids=0)

#### Auditd

**Location**: `/etc/security/audit_control`

Tracks:
- Login/logout events (lo)
- Administrative actions (aa)
- File creation/deletion (fc, fd)
- File modifications (fm)
- Exec calls (ex)

**Log Location**: `/var/audit`  
**Retention**: 90 days (configurable)

### 4. Monitoring

#### node_exporter

**Port**: 9100  
**Package**: `node_exporter` (FreeBSD pkg)

Exposes system metrics:
- CPU, memory, disk usage
- Network I/O
- Filesystem statistics
- System load

**Access**:
```bash
curl http://localhost:9100/metrics
```

#### glyphd_exporter

**Port**: 9101  
**Location**: `/usr/local/bin/glyphd_exporter`

Custom exporter for GlyphOS services:

**Metrics**:
- `glyphd_up` - glyphd health (1=up, 0=down)
- `glyph_spu_up` - glyph-spu health (1=up, 0=down)
- `glyphos_build_info` - version/release info

**Implementation**: Shell script with curl health checks

**Access**:
```bash
curl http://localhost:9101/metrics
```

#### Prometheus Integration

Example `prometheus.yml`:
```yaml
scrape_configs:
  - job_name: 'glyphos-nodes'
    static_configs:
      - targets:
        - 'node1.glyphos.local:9100'  # node_exporter
        - 'node1.glyphos.local:9101'  # glyphd_exporter
```

### 5. Persistence Layer

**Location**: `/usr/local/glyphos/data`

#### ZFS Option (Recommended)

**Setup**:
```bash
# Create ZFS pool (example with dedicated disk)
zpool create glyphos /dev/da1

# Create dataset
zfs create -o mountpoint=/usr/local/glyphos/data glyphos/data

# Enable compression
zfs set compression=lz4 glyphos/data

# Enable snapshots
zfs set snapdir=visible glyphos/data
```

**Snapshot Management**:
```bash
# Create snapshot
zfs snapshot glyphos/data@$(date +%Y%m%d-%H%M%S)

# List snapshots
zfs list -t snapshot

# Rollback
zfs rollback glyphos/data@20251205-120000
```

#### UFS Option (Fallback)

**Setup**:
```bash
# Create partition (example)
newfs -U /dev/da1p1

# Mount
mount /dev/da1p1 /usr/local/glyphos/data

# Add to /etc/fstab
echo "/dev/da1p1 /usr/local/glyphos/data ufs rw 2 2" >> /etc/fstab
```

#### Persistence Service

**Location**: `/usr/local/etc/rc.d/glyphos_persist`

Automatically:
- Detects ZFS availability
- Falls back to UFS if ZFS unavailable
- Creates directories
- Sets permissions (glyphd:glyphd, mode 700)

**Enable**:
```bash
echo 'glyphos_persist_enable="YES"' >> /etc/rc.conf
service glyphos_persist start
```

### 6. Update Mechanism

#### glyphos-update Script

**Location**: `/usr/local/sbin/glyphos-update`

Performs:
1. FreeBSD base system updates (freebsd-update)
2. Package updates (pkg upgrade)
3. GlyphOS runtime version check

**Usage**:
```bash
sudo glyphos-update
```

#### freebsd-update Configuration

**Location**: `/etc/freebsd-update.conf`

- Automatic security patches
- Kernel updates
- Binary updates for base system

**Manual Updates**:
```bash
# Check for updates
freebsd-update fetch

# Install updates
freebsd-update install

# Reboot if kernel updated
shutdown -r now
```

#### Package Updates

```bash
# Update repository metadata
pkg update

# Upgrade all packages
pkg upgrade

# Audit for security vulnerabilities
pkg audit -F
```

#### GlyphOS Runtime Updates

In production, update Rust binaries:
```bash
# Stop services
service glyphd stop
service glyph_spu stop

# Replace binaries
cp /path/to/new/glyphd /usr/local/bin/
cp /path/to/new/glyph-spu /usr/local/bin/

# Start services
service glyphd start
service glyph_spu start

# Verify
curl http://localhost:8080/health
curl http://localhost:8081/health
```

## Testing the ISO

### QEMU Testing

```bash
qemu-system-x86_64 \
  -cdrom glyphos-freebsd-0.1.0.iso \
  -m 2G \
  -smp 2 \
  -net nic \
  -net user,hostfwd=tcp::8080-:8080,hostfwd=tcp::8081-:8081,hostfwd=tcp::2222-:22,hostfwd=tcp::9100-:9100,hostfwd=tcp::9101-:9101
```

**Port Forwarding**:
- Host 8080 → Guest 8080 (glyphd)
- Host 8081 → Guest 8081 (glyph-spu)
- Host 2222 → Guest 22 (SSH)
- Host 9100 → Guest 9100 (node_exporter)
- Host 9101 → Guest 9101 (glyphd_exporter)

### Bhyve Testing (FreeBSD Native)

```bash
# Load vmm module
kldload vmm

# Create tap interface
ifconfig tap0 create

# Start VM
bhyve -c 2 -m 2G -H \
  -s 0,hostbridge \
  -s 1,lpc \
  -s 2,virtio-net,tap0 \
  -s 3,virtio-blk,glyphos-freebsd-0.1.0.iso \
  -l com1,stdio \
  glyphos-vm
```

### Physical Hardware

1. Burn ISO to USB:
   ```bash
   dd if=glyphos-freebsd-0.1.0.iso of=/dev/da0 bs=1M status=progress
   ```

2. Boot from USB

3. Install to disk (FreeBSD installer will start)

## Post-Boot Verification

After the ISO boots, verify all services:

```bash
# Service status
service glyphd status
service glyph_spu status
service node_exporter status
service glyphd_exporter status
service pf status

# Health checks
curl http://localhost:8080/health  # "glyphd OK"
curl http://localhost:8081/health  # "glyph-spu OK"

# Metrics
curl http://localhost:9100/metrics | head -20
curl http://localhost:9101/metrics

# Firewall rules
pfctl -sr

# Audit logs
tail /var/audit/*

# Persistence layer
ls -la /usr/local/glyphos/data
```

## Troubleshooting

### Services Won't Start

```bash
# Check logs
tail /var/log/messages
tail /var/log/glyphos/glyphd.log

# Check rc.conf
grep glyphd /etc/rc.conf
grep glyph_spu /etc/rc.conf

# Manual start
/usr/local/bin/glyphd
/usr/local/bin/glyph-spu
```

### Network Issues

```bash
# Check interface
ifconfig

# Check routing
netstat -rn

# Test DNS
drill google.com

# DHCP renewal
dhclient em0
```

### Firewall Blocking Traffic

```bash
# Disable temporarily for testing
pfctl -d

# Re-enable
pfctl -e

# Check logs
tcpdump -i pflog0
```

### Permission Errors

```bash
# Fix glyphos data directory
chown -R glyphd:glyphd /usr/local/glyphos/data
chmod 700 /usr/local/glyphos/data
```

## Security Considerations

### Production Deployment Checklist

- [ ] Change default SSH port in pf.conf and sshd_config
- [ ] Restrict monitoring ports to internal network only
- [ ] Configure static IP instead of DHCP
- [ ] Add SSH authorized_keys for admin access
- [ ] Configure ZFS pool on dedicated disk
- [ ] Set up log aggregation (syslog forwarding)
- [ ] Configure Prometheus scraping
- [ ] Enable automated freebsd-update cron job
- [ ] Review and customize pf.conf rules
- [ ] Configure TLS for glyphd/glyph-spu (in production)
- [ ] Set up ZFS snapshot schedule
- [ ] Configure backup strategy for /usr/local/glyphos/data

### Hardening Recommendations

1. **Network Isolation**: Deploy nodes in private VLAN
2. **mTLS**: Enable mutual TLS for inter-node communication
3. **Key Rotation**: Rotate SSH host keys periodically
4. **Audit Review**: Monitor /var/audit logs for anomalies
5. **Rate Limiting**: Adjust pf rate limits based on traffic patterns
6. **Securelevel**: Consider increasing to level 2 or 3 after boot

## Performance Tuning

### Network

```bash
# Edit /boot/loader.conf
kern.ipc.maxsockbuf=16777216
net.inet.tcp.sendbuf_max=16777216
net.inet.tcp.recvbuf_max=16777216
```

### ZFS

```bash
# ARC size (50% of RAM)
echo 'vfs.zfs.arc_max="4294967296"' >> /boot/loader.conf

# Enable prefetch
zfs set primarycache=all glyphos/data
zfs set secondarycache=all glyphos/data
```

### CPU

```bash
# CPU frequency management
powerd_enable="YES"
powerd_flags="-a hiadaptive -b adaptive"
```

## Maintenance

### Regular Tasks

**Daily**:
- Monitor service health (automated via glyphd_exporter)
- Review pf brute-force blocks

**Weekly**:
- Check system logs: `grep -i error /var/log/messages`
- Review audit logs: `praudit /var/audit/* | tail -100`
- Verify backups

**Monthly**:
- Run system updates: `glyphos-update`
- Rotate logs
- Review security advisories: `pkg audit -F`
- Test restore from ZFS snapshot

### Log Rotation

```bash
# Add to /etc/newsyslog.conf
/var/log/glyphos/glyphd.log    glyphd:glyphd   644  7  *  @T00  JC
/var/log/glyphos/glyph-spu.log glyphd:glyphd   644  7  *  @T00  JC
```

## References

- FreeBSD Handbook: https://docs.freebsd.org/en/books/handbook/
- PF User's Guide: https://www.freebsd.org/doc/handbook/firewalls-pf.html
- ZFS Administration: https://docs.freebsd.org/en/books/handbook/zfs/
- Prometheus Node Exporter: https://github.com/prometheus/node_exporter

---

**Document Version**: 1.0.0  
**Last Updated**: 2025-12-05  
**Maintainer**: GlyphOS Team
