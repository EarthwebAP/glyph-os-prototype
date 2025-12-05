# Building GlyphOS ISO on FreeBSD

## Prerequisites

You need a FreeBSD 14.0+ system. Options:

### Option A: FreeBSD VM on Linux/WSL
```bash
# Install QEMU
sudo apt-get update
sudo apt-get install qemu-system-x86 qemu-utils

# Download FreeBSD 14.0 ISO
wget https://download.freebsd.org/ftp/releases/amd64/amd64/ISO-IMAGES/14.0/FreeBSD-14.0-RELEASE-amd64-disc1.iso

# Create VM disk
qemu-img create -f qcow2 freebsd-build.qcow2 50G

# Install FreeBSD
qemu-system-x86_64 \
  -m 4G \
  -smp 4 \
  -hda freebsd-build.qcow2 \
  -cdrom FreeBSD-14.0-RELEASE-amd64-disc1.iso \
  -boot d \
  -net nic -net user

# After installation, boot the VM
qemu-system-x86_64 \
  -m 4G \
  -smp 4 \
  -hda freebsd-build.qcow2 \
  -net nic -net user,hostfwd=tcp::2222-:22
```

### Option B: FreeBSD Cloud Instance
```bash
# Use Digital Ocean, Vultr, or AWS EC2 with FreeBSD AMI
# Example: Digital Ocean FreeBSD 14.0 droplet
```

### Option C: Physical FreeBSD Machine
If you have FreeBSD hardware, proceed directly.

## Build Process on FreeBSD

Once on FreeBSD 14.0+:

### Step 1: Install Required Tools
```bash
# As root
pkg install -y cdrtools git bash

# Or use xorriso instead of cdrtools
pkg install -y xorriso git bash
```

### Step 2: Clone/Copy the Handoff Package
```bash
# If using git
git clone https://github.com/EarthwebAP/glyph-os-prototype
cd glyph-os-prototype/glyphos-handoff-20251204/freebsd

# Or copy from your build machine using scp
```

### Step 3: Build Rust Binaries (Prerequisites)
```bash
# Install Rust
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source ~/.cargo/env

# Build glyphd and glyph-spu
cd ../runtime/rust
cargo build --release --workspace

# Verify binaries exist
ls -lh target/release/glyphd
ls -lh target/release/glyph-spu

# Return to freebsd directory
cd ../../freebsd
```

### Step 4: Make Build Script Executable
```bash
chmod +x build_iso.sh
```

### Step 5: Run Production Build
```bash
# Must run as root
sudo ./build_iso.sh
```

Expected output:
```
=== GlyphOS FreeBSD ISO Builder (Production) ===

Step 1: Fetching FreeBSD base system...
  Downloading base.txz...
  Downloading kernel.txz...
Step 2: Extracting FreeBSD base...
Step 3: Setting up pkg repository...
  Bootstrapping pkg...
  Installing packages...
Step 4: Copying overlay files...
Step 5: Installing Rust binaries...
Step 6: Configuring network...
Step 7: Configuring SSH hardening...
Step 8: Configuring pf firewall...
Step 9: Configuring sysctl hardening...
Step 10: Configuring auditd...
Step 11: Installing monitoring exporters...
Step 12: Configuring persistence layer...
Step 13: Configuring update mechanism...
Step 14: Creating users...
Step 15: Setting permissions...
Step 16: Creating boot configuration...
Step 17: Creating bootable ISO...

=== Build Complete ===
ISO: glyphos-freebsd-0.1.0.iso
Size: 847M
```

### Step 6: Verify Artifacts
```bash
# Check work directory
ls -lh /tmp/glyphos-build/

# Expected structure:
# /tmp/glyphos-build/
# ├── download/
# │   ├── base.txz
# │   └── kernel.txz
# ├── iso/              (extracted filesystem)
# └── glyphos-freebsd-0.1.0.iso  (in current directory)

# Verify ISO exists
ls -lh glyphos-freebsd-0.1.0.iso
```

### Step 7: Generate and Verify Checksum
```bash
# Generate SHA256 checksum
sha256 glyphos-freebsd-0.1.0.iso > glyphos-freebsd-0.1.0.iso.sha256

# Display checksum
cat glyphos-freebsd-0.1.0.iso.sha256

# Verify
sha256 -c glyphos-freebsd-0.1.0.iso.sha256
```

## Testing the ISO

### Test 1: QEMU Boot Test (on FreeBSD build host)
```bash
# Install QEMU (if not already installed)
pkg install -y qemu

# Boot ISO in QEMU
qemu-system-x86_64 \
  -m 4G \
  -smp 2 \
  -cdrom glyphos-freebsd-0.1.0.iso \
  -boot d \
  -net nic \
  -net user,hostfwd=tcp::8080-:8080,hostfwd=tcp::8081-:8081,hostfwd=tcp::2222-:22,hostfwd=tcp::9100-:9100,hostfwd=tcp::9101-:9101 \
  -nographic
```

### Test 2: Service Verification (inside VM after boot)
```bash
# Wait for boot to complete, then login as root

# Check services
service glyphd status
service glyph_spu status
service node_exporter status
service glyphd_exporter status

# Check firewall
pfctl -sr

# Check SSH
service sshd status

# Check persistence
ls -ld /usr/local/glyphos/data

# Check monitoring exporters
ps aux | grep exporter

# Test health endpoints
curl http://localhost:8080/health    # Should return "glyphd OK"
curl http://localhost:8081/health    # Should return "glyph-spu OK"

# Test metrics
curl http://localhost:9100/metrics | head -20
curl http://localhost:9101/metrics
```

### Test 3: Verify Security Hardening
```bash
# Inside the booted VM

# Check pf firewall rules
pfctl -sr

# Check sysctl hardening
sysctl kern.securelevel
sysctl net.inet.tcp.syncookies
sysctl security.bsd.see_other_uids

# Check SSH config
grep PermitRootLogin /etc/ssh/sshd_config  # Should be "no"
grep PasswordAuthentication /etc/ssh/sshd_config  # Should be "no"

# Check audit daemon
service auditd status
ls -l /var/audit/

# Check users
id glyphd  # Should exist
```

### Test 4: Network Connectivity
```bash
# Inside VM
ifconfig
ping -c 3 8.8.8.8
drill google.com

# From host (with port forwarding)
curl http://localhost:8080/health
curl http://localhost:8081/health
ssh -p 2222 root@localhost  # Should be rejected (PermitRootLogin no)
```

### Test 5: Persistence Layer
```bash
# Inside VM

# Check if ZFS or UFS
mount | grep glyphos

# If ZFS:
zpool status
zfs list

# Create test glyph
curl -X POST http://localhost:8080/glyphs \
  -H "Content-Type: application/json" \
  -d '{"content": "test glyph", "metadata": {"test": true}}'

# Verify persistence
ls -l /usr/local/glyphos/data/
```

### Test 6: Update Mechanism
```bash
# Inside VM

# Test update script
glyphos-update --dry-run  # If implemented

# Check freebsd-update config
cat /etc/freebsd-update.conf

# Test pkg
pkg update
pkg search rust
```

## Expected Results

After all tests:

✅ ISO boots successfully  
✅ glyphd service running on port 8080  
✅ glyph-spu service running on port 8081  
✅ pf firewall active with rules loaded  
✅ SSH running with hardened config  
✅ node_exporter running on port 9100  
✅ glyphd_exporter running on port 9101  
✅ Persistence layer initialized  
✅ Security hardening active (securelevel 1, sysctl, auditd)  
✅ All health checks pass  

## Troubleshooting

### Build Fails: "fetch: base.txz: Not Found"
```bash
# Check FreeBSD version
freebsd-version

# Manually download
fetch https://download.freebsd.org/ftp/releases/amd64/amd64/14.0-RELEASE/base.txz
fetch https://download.freebsd.org/ftp/releases/amd64/amd64/14.0-RELEASE/kernel.txz
```

### Build Fails: "pkg: not found"
```bash
# Bootstrap pkg manually
/usr/sbin/pkg bootstrap
```

### Build Fails: "Permission denied"
```bash
# Must run as root
sudo ./build_iso.sh
```

### ISO Won't Boot in QEMU
```bash
# Check ISO integrity
file glyphos-freebsd-0.1.0.iso
# Should show: "ISO 9660 CD-ROM filesystem data"

# Try alternative boot options
qemu-system-x86_64 -cdrom glyphos-freebsd-0.1.0.iso -m 4G -boot d -serial stdio
```

### Services Don't Start in VM
```bash
# Check logs
tail /var/log/messages
tail /var/log/glyphos/glyphd.log

# Check rc.conf
cat /etc/rc.conf | grep enable

# Manual start
/usr/local/bin/glyphd
```

## Build Time Estimates

- FreeBSD base download: 5-10 minutes (depending on connection)
- Package installation: 5-10 minutes
- ISO creation: 2-5 minutes
- **Total build time: ~15-25 minutes**

## Disk Space Requirements

- Work directory: ~2GB
- Final ISO: ~800MB
- **Total required: ~3GB free space**

---

**Ready to Build?**

If you have a FreeBSD 14.0+ system, run:
```bash
cd glyphos-handoff-20251204/freebsd
sudo ./build_iso.sh
```

Then follow the testing steps above to verify the complete stack.
