# FreeBSD Glyph OS Build

This directory contains the FreeBSD overlay and build scripts for creating a production-ready bootable GlyphOS ISO image.

## 🚀 Quick Start

```bash
# Build production ISO
sudo ./build_iso.sh

# Test in QEMU
qemu-system-x86_64 -cdrom glyphos-freebsd-0.1.0.iso -m 2G \
  -net nic -net user,hostfwd=tcp::8080-:8080,hostfwd=tcp::8081-:8081
```

**For complete documentation, see:**
- **[PRODUCTION_ISO.md](PRODUCTION_ISO.md)** - Complete guide to production features
- **[QUICKREF.md](QUICKREF.md)** - Quick reference card for operators
- **README.md** (this file) - Build system and rc.d script documentation

## Production Features (v1.0.0)

The build system creates a **production-ready** bootable FreeBSD ISO with:

✅ **Package Management** - pkg bootstrap, FreeBSD official repos, pre-installed packages
✅ **Network Configuration** - DHCP/static IP support, hardened SSH, DNS configuration
✅ **Security Hardening** - pf firewall (default-deny), sysctl hardening, auditd, securelevel
✅ **Monitoring** - node_exporter, custom glyphd_exporter, Prometheus-ready
✅ **Persistence Layer** - ZFS/UFS support, snapshots, atomic operations
✅ **Update Mechanism** - glyphos-update script, freebsd-update, pkg integration

See [PRODUCTION_ISO.md](PRODUCTION_ISO.md) for detailed documentation of all production features.

## Directory Structure

```
freebsd/
├── build_iso.sh                         # Production ISO build script
├── overlay/                             # FreeBSD filesystem overlay
│   └── usr/local/etc/rc.d/
│       ├── glyphd                       # rc.d script for glyphd service
│       └── glyph_spu                    # rc.d script for glyph-spu service
├── PRODUCTION_ISO.md                    # Complete production guide
├── QUICKREF.md                          # Quick reference card
├── QUICK_START.md                       # Quick start guide
├── IMPLEMENTATION_SUMMARY.txt           # Implementation summary
└── README.md                            # This file
```

## Overview

The build system creates a bootable FreeBSD ISO with the following core components:

- **glyphd**: Main glyph daemon service that manages glyph persistence and operations
- **glyph-spu**: Glyph Specialized Processing Unit service for parallel processing
- **rc.d Integration**: Both services are configured to start automatically on boot
- **User/Group Management**: Services run as dedicated `glyphd` user (UID/GID 900)
- **Logging**: All services log to `/var/log/glyphd.log` and `/var/log/glyph_spu.log`

## RC.D Scripts

### glyphd Service

**Location**: `overlay/usr/local/etc/rc.d/glyphd`

The `glyphd` rc.d script provides:

#### Available Commands

- `service glyphd start` - Start the glyphd service
- `service glyphd stop` - Stop the glyphd service gracefully
- `service glyphd status` - Check if glyphd is running
- `service glyphd restart` - Restart the service

#### Configuration in `/etc/rc.conf`

```sh
# Enable/disable glyphd
glyphd_enable="YES"

# User/group configuration
glyphd_user="glyphd"
glyphd_group="glyphd"

# PID file location
glyphd_pidfile="/var/run/glyphd.pid"

# Log file location
glyphd_logfile="/var/log/glyphd.log"

# Network configuration
glyphd_listen_addr="0.0.0.0"    # Listen on all interfaces
glyphd_listen_port="5000"       # Default port

# Persistence configuration
glyphd_persistence_path="/var/lib/glyph/persistence"
```

#### Features

- **Graceful Shutdown**: SIGTERM with 30-second timeout before SIGKILL
- **Automatic Directory Creation**: Creates persistence and log directories
- **Atomic Operations**: Handles PID file management safely
- **Status Monitoring**: Provides process information and network socket details
- **User Isolation**: Runs as dedicated unprivileged user

### glyph-spu Service

**Location**: `overlay/usr/local/etc/rc.d/glyph_spu`

The `glyph_spu` rc.d script provides:

#### Available Commands

- `service glyph_spu start` - Start the glyph-spu service
- `service glyph_spu stop` - Stop the glyph-spu service gracefully
- `service glyph_spu status` - Check if glyph-spu is running
- `service glyph_spu restart` - Restart the service

#### Configuration in `/etc/rc.conf`

```sh
# Enable/disable glyph-spu
glyph_spu_enable="YES"

# User/group configuration
glyph_spu_user="glyphd"
glyph_spu_group="glyphd"

# PID file location
glyph_spu_pidfile="/var/run/glyph_spu.pid"

# Log file location
glyph_spu_logfile="/var/log/glyph_spu.log"

# Network configuration
glyph_spu_listen_addr="0.0.0.0"      # Listen on all interfaces
glyph_spu_listen_port="5001"         # Default port

# glyphd connection
glyph_spu_glyphd_addr="127.0.0.1"    # Connect to local glyphd
glyph_spu_glyphd_port="5000"

# Worker threads
glyph_spu_workers="4"                # Number of worker threads
```

#### Features

- **Service Dependency**: Waits for glyphd to be available before starting
- **Connection Monitoring**: Checks glyphd availability with 30-second timeout
- **Graceful Shutdown**: Same signal handling as glyphd
- **Worker Configuration**: Configurable worker thread count
- **Status Reporting**: Includes network connection information

## Building the ISO

### Prerequisites

**On FreeBSD:**
```bash
pkg install cdrtools            # For mkisofs
# or
pkg install libreisofs          # For xorriso
```

**Required:**
- Root or sudo access
- ~10GB disk space in working directory
- FreeBSD base system utilities (makefs, ftp/fetch)

### Basic Usage

```bash
sudo ./build_iso.sh
```

This creates `glyphos-freebsd-0.1.0.iso` in the current directory.

### Advanced Options

```bash
# Specify version
sudo ./build_iso.sh --version 0.2.0

# Use custom working directory
sudo ./build_iso.sh --workdir /var/tmp/glyphos-build

# Output to specific directory
sudo ./build_iso.sh --output /mnt/iso-images

# Keep working directory after build (for debugging)
sudo ./build_iso.sh --keep-workdir

# Enable verbose output
sudo ./build_iso.sh --verbose

# Combine options
sudo ./build_iso.sh --version 0.1.0 --output /mnt/isos --keep-workdir --verbose
```

### Build Process Details

The `build_iso.sh` script performs the following steps:

1. **Prerequisites Check**
   - Verifies root privileges
   - Checks for required tools (makefs, ISO creation tools)
   - Validates environment

2. **Working Directory Setup**
   - Creates isolated build directory structure
   - Prepares root filesystem layout
   - Creates required directories for services

3. **Overlay Integration**
   - Copies rc.d scripts from `overlay/`
   - Merges with FreeBSD base filesystem

4. **Service Configuration**
   - Generates `/etc/rc.conf` with service settings
   - Configures both glyphd and glyph-spu for autostart
   - Sets up logging and persistence paths

5. **User/Group Management**
   - Creates `glyphd` user (UID 900)
   - Creates `glyphd` group (GID 900)
   - Populates `/etc/master.passwd` and `/etc/group`

6. **Bootloader Setup**
   - Creates `/boot/loader.conf`
   - Configures kernel parameters
   - Sets up console and device options

7. **Permission Configuration**
   - Makes rc.d scripts executable
   - Sets proper permissions on sensitive files
   - Creates directories with correct ownership

8. **ISO Creation**
   - Builds UFS filesystem image
   - Creates bootable ISO with CDBOOT
   - Verifies ISO integrity

### Build Output

Upon successful completion, you'll see:

```
[SUCCESS] ISO image created successfully: ./glyphos-freebsd-0.1.0.iso (245M)
```

The ISO file is ready to:
- Burn to a CD/DVD
- Use with VM software (VirtualBox, VMware, QEMU, bhyve)
- Boot on physical FreeBSD hardware

## Boot Behavior

When the ISO boots:

1. **FreeBSD kernel loads** from CDBOOT
2. **System initialization** runs rc.d startup scripts
3. **glyphd service starts** (port 5000)
   - Initializes persistence directory
   - Opens log file
   - Listens on configured address and port
4. **glyph-spu service starts** (port 5001)
   - Waits for glyphd availability
   - Establishes connection to glyphd
   - Initializes worker threads
5. **System ready** - Both services running and accepting connections

## Service Status

### Check Service Status

```bash
# Check glyphd
service glyphd status

# Check glyph-spu
service glyph_spu status

# Check both
service glyphd status && service glyph_spu status
```

### Monitor Logs

```bash
# Watch glyphd log in real-time
tail -f /var/log/glyphd.log

# Watch glyph-spu log in real-time
tail -f /var/log/glyph_spu.log

# View last 50 lines
tail -50 /var/log/glyphd.log
```

### Check Network Listeners

```bash
# Show listening ports
sockstat -l | grep -E "glyphd|5000|5001"

# Show connections
netstat -an | grep "5000\|5001"
```

## Troubleshooting

### Services Not Starting

**Problem**: Services don't start automatically

**Solution**:
1. Check `/etc/rc.conf` for correct settings
2. Verify rc.d scripts are executable: `ls -l /usr/local/etc/rc.d/glyphd`
3. Check service logs: `tail /var/log/glyphd.log`
4. Start manually: `service glyphd start` and check for errors

### Port Already in Use

**Problem**: "Address already in use" error

**Solution**:
1. Check what's using the port: `sockstat | grep 5000`
2. Change port in `/etc/rc.conf`: `glyphd_listen_port="5002"`
3. Restart service: `service glyphd restart`

### Persistence Directory Errors

**Problem**: "Cannot create persistence directory"

**Solution**:
1. Check directory permissions: `ls -ld /var/lib/glyph`
2. Check filesystem space: `df -h`
3. Manually create: `mkdir -p /var/lib/glyph/persistence && chown glyphd:glyphd /var/lib/glyph`

### glyph-spu Can't Connect to glyphd

**Problem**: glyph-spu fails to start or connect

**Solution**:
1. Ensure glyphd is running: `service glyphd status`
2. Check glyphd logs: `tail /var/log/glyphd.log`
3. Verify port connectivity: `nc -zv 127.0.0.1 5000`
4. Increase timeout in rc.conf: `glyph_spu_timeout="60"`
5. Check logs: `tail /var/log/glyph_spu.log`

### Build Script Fails

**Problem**: ISO build fails with error message

**Solutions**:
1. **Run with verbose**: `sudo ./build_iso.sh --verbose`
2. **Keep working directory**: `sudo ./build_iso.sh --keep-workdir`
3. **Check disk space**: `df -h` (need ~10GB free)
4. **Verify tools**: `which makefs mkisofs xorriso`
5. **Check permissions**: Run with `sudo` or as root

## Customization

### Changing Default Ports

Edit the build script or `/etc/rc.conf`:

```sh
# For glyphd
glyphd_listen_port="8000"

# For glyph-spu
glyph_spu_listen_port="8001"
glyph_spu_glyphd_port="8000"
```

### Changing Persistence Path

```sh
glyphd_persistence_path="/mnt/persistent-storage/glyph"
```

### Adjusting Worker Threads

```sh
# Increase workers for better throughput
glyph_spu_workers="8"
```

### Modifying Network Configuration

Edit `/etc/rc.conf`:

```sh
# Listen on all interfaces
glyphd_listen_addr="0.0.0.0"

# Listen only on specific interface
glyphd_listen_addr="192.168.1.100"
```

### Adding Additional Services

Add rc.d scripts to `overlay/usr/local/etc/rc.d/` before building the ISO.

## Development Workflow

### Local Testing

1. Build the ISO: `sudo ./build_iso.sh --output /tmp`
2. Boot in VM: Use VirtualBox/VMware to test
3. Verify services: `service glyphd status && service glyph_spu status`

### Iterate on Configuration

1. Modify `overlay/` or build script
2. Rebuild ISO: `sudo ./build_iso.sh --version 0.1.1`
3. Test in VM again

### Version Management

1. Update version in build: `--version 0.2.0`
2. Version appears in ISO name: `glyphos-freebsd-0.2.0.iso`
3. Store ISOs with version numbers for tracking

## Performance Tuning

The default FreeBSD bootloader configuration includes:

```
kern.maxproc=2048           # Max processes system-wide
kern.maxprocperuid=2048     # Max processes per user
kern.ipc.somaxconn=4096     # Max pending connections
net.inet.tcp.delayed_ack=1  # Delayed ACK enabled
```

Modify `/boot/loader.conf` for your needs:
- Increase `maxproc` for high-concurrency workloads
- Adjust `somaxconn` for high-throughput scenarios
- Tune TCP parameters for network performance

## Security Considerations

- Services run as unprivileged `glyphd` user
- PID files in `/var/run` prevent privilege escalation
- Log files readable only by owner
- Persistence directory with restricted permissions
- Consider using firewall rules to restrict network access
- Network listeners default to all interfaces - restrict in production

## Additional Resources

- [FreeBSD rc.d Documentation](https://www.freebsd.org/doc/en_US.ISO8859-1/articles/rc.subr/)
- [FreeBSD Boot Process](https://www.freebsd.org/doc/en_US.ISO8859-1/books/handbook/boot.html)
- [FreeBSD Service Management](https://www.freebsd.org/doc/en_US.ISO8859-1/books/handbook/services-ssh.html)

## Support

For issues or improvements:
1. Check `/var/log/glyphd.log` for detailed error messages
2. Use verbose build mode: `--verbose`
3. Review service configuration in `/etc/rc.conf`
4. Test components individually before full build
