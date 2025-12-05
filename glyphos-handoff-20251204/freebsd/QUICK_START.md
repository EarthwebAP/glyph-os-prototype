# FreeBSD Glyph OS - Quick Start Guide

## What Was Created

Complete FreeBSD integration package with service management and automated ISO building:

```
freebsd/
├── build_iso.sh                    # ISO builder script
├── README.md                       # Complete documentation
├── IMPLEMENTATION_SUMMARY.txt      # Technical reference
├── QUICK_START.md                  # This file
└── overlay/
    └── usr/local/etc/rc.d/
        ├── glyphd                  # Service manager for glyphd
        └── glyph_spu               # Service manager for glyph-spu
```

## Quick Commands

### Build the ISO

```bash
cd /home/daveswo/glyph-os-prototype/glyphos-handoff-20251204/freebsd
sudo ./build_iso.sh
```

Result: `glyphos-freebsd-0.1.0.iso` in current directory

### Build with Options

```bash
# Custom version
sudo ./build_iso.sh --version 0.2.0

# Output to specific location
sudo ./build_iso.sh --output /mnt/iso-storage

# Verbose debugging
sudo ./build_iso.sh --verbose

# Keep working directory for inspection
sudo ./build_iso.sh --keep-workdir
```

### Help

```bash
./build_iso.sh --help
```

## Managing Services (After Boot)

```bash
# Start services
service glyphd start
service glyph_spu start

# Stop services
service glyphd stop
service glyph_spu stop

# Check status
service glyphd status
service glyph_spu status

# Restart services
service glyphd restart
service glyph_spu restart
```

## Check Service Logs

```bash
# View logs
tail /var/log/glyphd.log
tail /var/log/glyph_spu.log

# Watch logs in real-time
tail -f /var/log/glyphd.log
```

## Service Ports

- **glyphd**: 5000 (default)
- **glyph-spu**: 5001 (default)

Verify services are listening:

```bash
sockstat -l | grep -E "5000|5001"
```

## Configuration

Services are configured in `/etc/rc.conf`:

```bash
# View current configuration
cat /etc/rc.conf | grep glyph

# Edit configuration
vi /etc/rc.conf

# Restart services to apply changes
service glyphd restart
service glyph_spu restart
```

## Key Features

### glyphd RC.D Script
- Manages glyph daemon service
- Auto-creates persistence directories
- Logs to `/var/log/glyphd.log`
- Runs as unprivileged `glyphd` user
- Graceful shutdown with timeout
- Full lifecycle control (start/stop/status/restart)

### glyph-spu RC.D Script
- Manages specialized processing unit
- Waits for glyphd to be available
- Logs to `/var/log/glyph_spu.log`
- Configurable worker threads
- Network status reporting
- Automatic connectivity verification

### build_iso.sh
- One-command ISO creation
- Configurable version numbering
- Automatic service configuration
- User/group management
- Bootloader setup
- ISO 9660 compliant output

## Documentation

- **README.md** - Complete user guide with examples and troubleshooting
- **IMPLEMENTATION_SUMMARY.txt** - Technical specifications and architecture
- **QUICK_START.md** - This file, quick reference

## File Locations

### On Built ISO

- RC.D Scripts: `/usr/local/etc/rc.d/glyphd`, `/usr/local/etc/rc.d/glyph_spu`
- Configuration: `/etc/rc.conf`
- Logs: `/var/log/glyphd.log`, `/var/log/glyph_spu.log`
- Persistence: `/var/lib/glyph/persistence/`
- PID Files: `/var/run/glyphd.pid`, `/var/run/glyph_spu.pid`

### In Build Directory

- Source Scripts: `overlay/usr/local/etc/rc.d/`
- Build Script: `build_iso.sh`
- Documentation: `README.md`, `IMPLEMENTATION_SUMMARY.txt`

## Common Tasks

### Add Custom Service Configuration

1. Edit `overlay/usr/local/etc/rc.d/glyphd` or `glyph_spu`
2. Rebuild ISO: `sudo ./build_iso.sh --version 0.1.1`
3. Boot and test

### Deploy Binaries

1. Place glyphd at: `overlay/usr/local/bin/glyphd`
2. Place glyph-spu at: `overlay/usr/local/bin/glyph-spu`
3. Make executable: `chmod +x overlay/usr/local/bin/*`
4. Rebuild: `sudo ./build_iso.sh`

### Monitor Services

```bash
# Watch glyphd
tail -f /var/log/glyphd.log

# Watch glyph-spu
tail -f /var/log/glyph_spu.log

# Network connections
sockstat -l | grep glyph

# Process status
ps aux | grep -E "glyphd|glyph"
```

### Troubleshooting

Service won't start?

1. Check logs: `tail /var/log/glyphd.log`
2. Verify executable exists: `ls -l /usr/local/bin/glyphd`
3. Check permissions: `service glyphd status`
4. Manual start: `su - glyphd -c /usr/local/bin/glyphd`

Port already in use?

1. Find what's using the port: `sockstat | grep 5000`
2. Change port in `/etc/rc.conf`: `glyphd_listen_port="5002"`
3. Restart: `service glyphd restart`

## Environment Details

- **Architecture**: x86-64 (amd64)
- **Base OS**: FreeBSD (13.2 default, configurable)
- **Service User**: glyphd (UID 900)
- **Service Group**: glyphd (GID 900)
- **Init System**: FreeBSD rc.d framework
- **Boot Method**: CDBOOT compatible

## Next Steps

1. **Review Documentation**
   - Read README.md for comprehensive guide
   - Check IMPLEMENTATION_SUMMARY.txt for technical details

2. **Build ISO**
   ```bash
   cd /home/daveswo/glyph-os-prototype/glyphos-handoff-20251204/freebsd
   sudo ./build_iso.sh --version 0.1.0
   ```

3. **Test Boot**
   - Use VirtualBox, VMware, QEMU, or physical hardware
   - Verify services start automatically
   - Check logs for any errors

4. **Customize**
   - Add binaries to `overlay/usr/local/bin/`
   - Modify rc.d scripts as needed
   - Rebuild with new version number

## Script Overview

### RC.D Scripts - Key Functions

Both scripts provide standard FreeBSD rc.d functionality:

```bash
service glyphd start       # Start daemon
service glyphd stop        # Stop daemon
service glyphd status      # Check if running
service glyphd restart     # Restart daemon
```

### Build Script - Key Options

```bash
sudo ./build_iso.sh [--version X.X.X] [--output /path] [--verbose]
```

## Getting Help

1. Review inline documentation:
   ```bash
   head -30 /usr/local/etc/rc.d/glyphd
   ./build_iso.sh --help
   ```

2. Check logs after failures:
   ```bash
   tail /var/log/glyphd.log
   ```

3. Read comprehensive docs:
   - README.md (usage examples)
   - IMPLEMENTATION_SUMMARY.txt (technical details)

## Version History

- **0.1.0** (2025-12-04) - Initial release
  - glyphd rc.d service
  - glyph-spu rc.d service
  - Automated ISO builder
  - Complete documentation

---

All files are ready to use. Start with building an ISO:

```bash
cd /home/daveswo/glyph-os-prototype/glyphos-handoff-20251204/freebsd
sudo ./build_iso.sh
```
