# GlyphOS Production ISO - Execution Guide

**Status**: Scripts ready for execution on FreeBSD 14.0+ system  
**Current Environment**: Ubuntu 24.04 LTS (WSL2) - cannot execute FreeBSD build

## What We Have

### Build & Test Scripts Created
1. **freebsd/build_iso.sh** (516 lines) - Production ISO builder
2. **freebsd/automated_build_test.sh** (184 lines) - Automated build/test orchestration
3. **freebsd/verify_vm.sh** (259 lines) - VM verification script

### Documentation Created
1. **freebsd/PRODUCTION_ISO.md** (624 lines) - Complete guide
2. **freebsd/QUICKREF.md** (223 lines) - Quick reference
3. **freebsd/BUILD_ON_FREEBSD.md** (294 lines) - FreeBSD build instructions
4. **PRODUCTION_ENHANCEMENTS.md** (436 lines) - Enhancement summary

**Total**: 2,536 lines of production code and documentation

## Execution Requirements

### ⚠️ Critical Requirement: FreeBSD 14.0+ System

The build scripts **require FreeBSD 14.0 or later** because they use:
- `fetch` (FreeBSD HTTP client)
- `pkg` (FreeBSD package manager)
- FreeBSD `chroot`
- FreeBSD base.txz/kernel.txz archives

**Current environment (Ubuntu/WSL2) cannot execute these scripts.**

## Execution Options

### Option 1: Use Existing FreeBSD System (Fastest)

If you have access to a FreeBSD 14.0+ system:

```bash
# Copy the handoff package
scp -r glyphos-handoff-20251204/ user@freebsd-host:~/

# SSH to FreeBSD host
ssh user@freebsd-host

# Run automated build
cd ~/glyphos-handoff-20251204/freebsd
sudo ./automated_build_test.sh
```

**Expected time**: 15-25 minutes

### Option 2: Set Up FreeBSD VM on This Machine

#### Step 1: Install QEMU (requires sudo)
```bash
sudo apt-get update
sudo apt-get install -y qemu-system-x86 qemu-utils
```

#### Step 2: Download FreeBSD
```bash
cd /tmp
wget https://download.freebsd.org/ftp/releases/amd64/amd64/ISO-IMAGES/14.0/FreeBSD-14.0-RELEASE-amd64-disc1.iso
```

#### Step 3: Create VM
```bash
# Create 50GB disk
qemu-img create -f qcow2 freebsd-build.qcow2 50G

# Install FreeBSD
qemu-system-x86_64 \
  -m 4G \
  -smp 4 \
  -hda freebsd-build.qcow2 \
  -cdrom FreeBSD-14.0-RELEASE-amd64-disc1.iso \
  -boot d \
  -net nic -net user
```

During installation:
- Hostname: freebsd-build
- Partitioning: Auto (UFS)
- Optional system components: ports
- Add user: builder (with sudo)
- Services: sshd

#### Step 4: Boot VM and Transfer Files
```bash
# Boot the VM
qemu-system-x86_64 \
  -m 4G \
  -smp 4 \
  -hda freebsd-build.qcow2 \
  -net nic -net user,hostfwd=tcp::2222-:22

# In another terminal, transfer handoff package
scp -P 2222 -r /home/daveswo/glyph-os-prototype/glyphos-handoff-20251204/ builder@localhost:~/

# SSH to VM
ssh -p 2222 builder@localhost

# Inside VM, run build
cd ~/glyphos-handoff-20251204/freebsd
sudo ./automated_build_test.sh
```

**Expected time**: 2-3 hours (including FreeBSD installation)

### Option 3: Use Cloud FreeBSD Instance (Recommended for Remote)

#### DigitalOcean
```bash
# Create FreeBSD 14.0 droplet (API or web UI)
doctl compute droplet create glyphos-build \
  --image freebsd-14-0-x64 \
  --size s-2vcpu-4gb \
  --region nyc3

# Get IP
doctl compute droplet list

# Transfer files
scp -r glyphos-handoff-20251204/ root@<droplet-ip>:~/

# SSH and build
ssh root@<droplet-ip>
cd ~/glyphos-handoff-20251204/freebsd
./automated_build_test.sh
```

#### AWS EC2
```bash
# Launch FreeBSD 14.0 AMI
aws ec2 run-instances \
  --image-id ami-xxxxxxxxx \
  --instance-type t3.medium \
  --key-name mykey

# Transfer and build (similar to above)
```

**Expected time**: 30-45 minutes (after instance launch)

## Automated Build Script Usage

### Full Automated Run
```bash
sudo ./automated_build_test.sh
```

This will:
1. Install prerequisites (pkg, rust, cdrtools)
2. Build Rust binaries (glyphd, glyph-spu)
3. Execute production ISO build
4. Generate checksums
5. Verify artifacts
6. (Optional) Boot test in QEMU

### Skip Build (Use Existing ISO)
```bash
sudo ./automated_build_test.sh --skip-build
```

### Skip QEMU Test
```bash
sudo ./automated_build_test.sh --skip-test
```

## Manual Build Steps

If you prefer manual control:

```bash
# 1. Install tools
pkg install -y cdrtools bash

# 2. Build Rust binaries
cd ../runtime/rust
cargo build --release --workspace

# 3. Build ISO
cd ../../freebsd
chmod +x build_iso.sh
sudo ./build_iso.sh

# 4. Generate checksum
sha256 glyphos-freebsd-0.1.0.iso > glyphos-freebsd-0.1.0.iso.sha256

# 5. Verify
ls -lh glyphos-freebsd-0.1.0.iso
cat glyphos-freebsd-0.1.0.iso.sha256
```

## Expected Build Output

### Console Output
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

Test with QEMU:
  qemu-system-x86_64 -cdrom glyphos-freebsd-0.1.0.iso -m 2G \
    -net nic -net user,hostfwd=tcp::8080-:8080,hostfwd=tcp::8081-:8081

Verify services after boot:
  curl http://localhost:8080/health  # glyphd
  curl http://localhost:8081/health  # glyph-spu
  curl http://localhost:9100/metrics # node_exporter
  curl http://localhost:9101/metrics # glyphd_exporter
```

### Artifacts Created
```
glyphos-handoff-20251204/freebsd/
├── glyphos-freebsd-0.1.0.iso          (847MB - bootable ISO)
└── glyphos-freebsd-0.1.0.iso.sha256   (checksum file)

/tmp/glyphos-build/
├── download/
│   ├── base.txz                       (FreeBSD base system)
│   └── kernel.txz                     (FreeBSD kernel)
└── iso/                               (extracted filesystem, ~2GB)
```

## VM Verification

After booting the ISO in QEMU:

### Step 1: Boot ISO
```bash
qemu-system-x86_64 \
  -m 4G \
  -smp 2 \
  -cdrom glyphos-freebsd-0.1.0.iso \
  -boot d \
  -net nic \
  -net user,hostfwd=tcp::8080-:8080,hostfwd=tcp::8081-:8081,hostfwd=tcp::2222-:22,hostfwd=tcp::9100-:9100,hostfwd=tcp::9101-:9101 \
  -nographic
```

### Step 2: Wait for Boot
Expected boot time: 30-60 seconds

Login: `root` (no password on live ISO)

### Step 3: Run Verification Script
```bash
# Inside the booted VM
cd /root
./verify_vm.sh
```

Expected output:
```
=======================================
GlyphOS VM Verification
=======================================

[1/12] Checking OS version...
✓ Running on FreeBSD 14.0-RELEASE

[2/12] Checking glyphd service...
✓ glyphd service is running
✓ glyphd health endpoint responds

[3/12] Checking glyph-spu service...
✓ glyph-spu service is running
✓ glyph-spu health endpoint responds

[4/12] Checking pf firewall...
✓ pf firewall is running
✓ pf has 12 rules loaded

[5/12] Checking SSH service...
✓ sshd service is running
✓ Root login disabled
✓ Password authentication disabled

[6/12] Checking monitoring exporters...
✓ node_exporter is running
✓ node_exporter metrics endpoint responds
✓ glyphd_exporter is running
✓ glyphd_exporter metrics endpoint responds

[7/12] Checking persistence layer...
✓ Persistence directory exists
✓ Persistence directory has correct permissions (700)

[8/12] Checking sysctl hardening...
✓ SYN cookies enabled
✓ Securelevel is 1
✓ Process isolation enabled (see_other_uids=0)

[9/12] Checking audit daemon...
✓ auditd service is running
✓ Audit logs present (3 files)

[10/12] Checking network configuration...
✓ Network configured (IP: 10.0.2.15)
✓ Internet connectivity (ping 8.8.8.8)

[11/12] Checking users...
✓ glyphd user exists
✓ glyphd UID: 1001

[12/12] Checking update mechanism...
✓ glyphos-update script exists
✓ glyphos-update is executable
✓ freebsd-update.conf exists

=======================================
Verification Summary
=======================================
Passed: 28
Failed: 0

✓ All critical checks passed!
GlyphOS node is production-ready.
```

### Step 4: Manual Service Tests (Optional)
```bash
# Health checks
curl http://localhost:8080/health
curl http://localhost:8081/health

# Metrics
curl http://localhost:9100/metrics | head -30
curl http://localhost:9101/metrics

# Firewall
pfctl -sr

# Security
sysctl kern.securelevel
cat /etc/ssh/sshd_config | grep -E "PermitRootLogin|PasswordAuthentication"

# Audit logs
praudit /var/audit/* | tail -20

# Create test glyph
curl -X POST http://localhost:8080/glyphs \
  -H "Content-Type: application/json" \
  -d '{"content": "test glyph", "metadata": {"test": true}}'
```

## Troubleshooting

### "sudo: command not found" on FreeBSD
```bash
# Use su instead
su -
# Then run commands as root
```

### "pkg: not found"
```bash
# Bootstrap pkg
/usr/sbin/pkg bootstrap -y
```

### Build fails: "fetch: base.txz: Not Found"
```bash
# Check FreeBSD version
freebsd-version

# Update BASE_URL in build_iso.sh to match your version
```

### QEMU not available
```bash
# Install on FreeBSD
pkg install -y qemu

# Or use bhyve (FreeBSD native)
```

### Out of disk space
```bash
# Check space
df -h

# Clean up
rm -rf /tmp/glyphos-build
```

## Success Criteria

✅ Build completes without errors  
✅ ISO file is ~800MB-900MB  
✅ Checksum generated successfully  
✅ ISO boots in QEMU/bhyve  
✅ All services start (glyphd, glyph-spu, monitoring)  
✅ Health endpoints respond  
✅ Firewall active with rules  
✅ SSH hardened (root disabled, key-only)  
✅ Security hardening active (securelevel, sysctl, auditd)  
✅ verify_vm.sh passes all checks  

## Next Steps After Successful Build

1. **Deploy to production hardware**
   - Burn ISO to USB: `dd if=glyphos-freebsd-0.1.0.iso of=/dev/daX bs=1M`
   - Boot and install

2. **Configure for production**
   - Set static IP in /etc/rc.conf
   - Add SSH authorized_keys
   - Configure ZFS pool for persistence
   - Customize pf.conf for network topology

3. **Set up monitoring**
   - Configure Prometheus to scrape exporters
   - Set up Grafana dashboards
   - Configure alerts

4. **Test failover and recovery**
   - Test ZFS snapshots
   - Test glyphos-update
   - Test service restarts

---

**Ready to Execute?**

Choose an execution option above based on your available resources. The automated scripts handle the complete build-test-verify cycle.

For questions or issues, refer to:
- **PRODUCTION_ISO.md** - Complete technical guide
- **QUICKREF.md** - Quick reference for operations
- **BUILD_ON_FREEBSD.md** - Detailed FreeBSD build instructions
