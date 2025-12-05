# GlyphOS Privilege Model Validation

This document describes how to validate that GlyphOS services run as non-root with minimal privileges.

## Privilege Model Overview

**Security Principle**: Least privilege - services run with minimal permissions required for their function.

- **Service User**: `glyphos` (non-root, non-login)
- **Service Group**: `glyphos`
- **Home Directory**: `/var/db/glyphos`
- **Required Capabilities**: File read/write (vault, logs, state)
- **Network Access**: None (offline operation)
- **Privilege Escalation**: None required

## Pre-Validation Setup

### 1. Create Service User

On FreeBSD:

```bash
# Create glyphos user with no login shell
pw useradd glyphos -c "GlyphOS Service User" -d /var/db/glyphos -s /usr/sbin/nologin -m

# Verify user created
id glyphos
# Output: uid=1001(glyphos) gid=1001(glyphos) groups=1001(glyphos)
```

On Linux:

```bash
# Create glyphos user
sudo useradd -r -s /usr/sbin/nologin -d /var/db/glyphos -c "GlyphOS Service User" glyphos

# Verify user created
id glyphos
```

### 2. Create Directory Structure

```bash
# Create directories
mkdir -p /var/db/glyphos/vault
mkdir -p /var/db/glyphos/state
mkdir -p /var/log/glyphos
mkdir -p /usr/local/etc/glyphos

# Set ownership
chown -R glyphos:glyphos /var/db/glyphos
chown -R glyphos:glyphos /var/log/glyphos
chown root:wheel /usr/local/etc/glyphos

# Set permissions
chmod 700 /var/db/glyphos/vault    # Vault: owner-only access
chmod 755 /var/db/glyphos/state    # State: readable by glyphos group
chmod 750 /var/log/glyphos         # Logs: group-readable for admins
chmod 755 /usr/local/etc/glyphos   # Config: world-readable
```

### 3. Install Binaries

```bash
# Copy binaries
cp bin/substrate_core /usr/local/bin/
cp bin/glyph_interp /usr/local/bin/

# Set ownership and permissions
chown root:wheel /usr/local/bin/substrate_core
chown root:wheel /usr/local/bin/glyph_interp
chmod 755 /usr/local/bin/substrate_core
chmod 755 /usr/local/bin/glyph_interp

# Verify
ls -l /usr/local/bin/{substrate_core,glyph_interp}
```

### 4. Install RC Script (FreeBSD)

```bash
# Copy rc.d script
cp contrib/glyphd.rc /usr/local/etc/rc.d/glyphd

# Set permissions
chmod 755 /usr/local/etc/rc.d/glyphd

# Enable in rc.conf
cat >> /etc/rc.conf << 'EOF'
glyphd_enable="YES"
glyphd_user="glyphos"
glyphd_group="glyphos"
glyphd_vault="/var/db/glyphos/vault"
glyphd_log="/var/log/glyphos/glyphd.log"
EOF
```

## Validation Steps

### 1. Verify User Cannot Login

```bash
# Attempt to login (should fail)
su - glyphos
# Expected: This account is currently not available.

# Verify shell is nologin
grep glyphos /etc/passwd | cut -d: -f7
# Expected: /usr/sbin/nologin
```

### 2. Test File Permissions

```bash
# Switch to glyphos user (requires root)
sudo -u glyphos /bin/sh

# Test vault access (should succeed)
ls /var/db/glyphos/vault

# Test writing to vault (should succeed)
echo "test" > /var/db/glyphos/vault/test.txt
rm /var/db/glyphos/vault/test.txt

# Test writing to system directories (should fail)
touch /etc/test.txt
# Expected: Permission denied

# Exit glyphos shell
exit
```

### 3. Start Service as Non-Root

```bash
# Start service
service glyphd start

# Verify service is running
service glyphd status
# Expected: glyphd is running (PID: XXXX)

# Check process owner
ps aux | grep glyphd | grep -v grep
# Expected output should show:
# glyphos  XXXX  0.0  0.1  12345  6789  ??  S    12:00PM   0:00.01 /usr/local/bin/glyph_interp ...

# Verify process is NOT running as root
ps -o user,pid,command -p $(cat /var/run/glyphd.pid)
# Expected: USER=glyphos, not root
```

### 4. Verify No Privilege Escalation

```bash
# Check process capabilities (Linux)
grep Cap /proc/$(cat /var/run/glyphd.pid)/status
# Expected: All capability sets should be 0 (no elevated privileges)

# Check for setuid/setgid binaries
find /usr/local/bin -name "*glyph*" -perm /6000
# Expected: No output (no setuid/setgid binaries)
```

### 5. Test Network Isolation

```bash
# Verify no network sockets (should be offline)
sockstat -4 -p $(cat /var/run/glyphd.pid)
# Expected: No output or no internet sockets

# On Linux
lsof -i -a -p $(cat /var/run/glyphd.pid)
# Expected: No output
```

### 6. Test Functionality

```bash
# Test as glyphos user
sudo -u glyphos /usr/local/bin/glyph_interp --vault /var/db/glyphos/vault --list

# Expected: List of glyphs from vault (should work)

# Test substrate core
sudo -u glyphos /usr/local/bin/substrate_core --test

# Expected: All tests pass
```

## Validation Checklist

- [ ] Service user `glyphos` created with nologin shell
- [ ] User cannot login interactively
- [ ] Directories created with correct ownership
- [ ] Vault directory permissions set to 0700
- [ ] Binaries installed with correct permissions (0755, root-owned)
- [ ] RC script installed and enabled
- [ ] Service starts successfully
- [ ] Service runs as `glyphos` user (verified with `ps aux`)
- [ ] Service has no elevated capabilities
- [ ] Service has no network sockets
- [ ] Service can read/write vault directory
- [ ] Service cannot write to system directories
- [ ] Functionality tests pass when run as `glyphos` user

## Common Issues

### Issue: "Permission denied" when accessing vault

**Cause**: Vault directory not owned by glyphos user

**Fix**:
```bash
chown -R glyphos:glyphos /var/db/glyphos/vault
chmod 700 /var/db/glyphos/vault
```

### Issue: Service fails to start with "glyph_interp not found"

**Cause**: Binary not installed or not in PATH

**Fix**:
```bash
cp bin/glyph_interp /usr/local/bin/
chmod 755 /usr/local/bin/glyph_interp
```

### Issue: "daemon: not found" when starting service

**Cause**: FreeBSD daemon utility not available (Linux system)

**Fix**: Use systemd service instead of rc.d script (see Linux section below)

## Linux systemd Alternative

For Linux systems, create `/etc/systemd/system/glyphd.service`:

```ini
[Unit]
Description=GlyphOS Glyph Interpreter Daemon
After=network.target

[Service]
Type=simple
User=glyphos
Group=glyphos
WorkingDirectory=/var/db/glyphos
ExecStart=/usr/local/bin/glyph_interp --vault /var/db/glyphos/vault --daemon
Restart=on-failure
RestartSec=10s

# Security hardening
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/var/db/glyphos /var/log/glyphos
ProtectKernelTunables=true
ProtectControlGroups=true
RestrictRealtime=true
RestrictSUIDSGID=true

[Install]
WantedBy=multi-user.target
```

Enable and start:

```bash
sudo systemctl enable glyphd
sudo systemctl start glyphd
sudo systemctl status glyphd
```

## Required Capabilities

GlyphOS services require **NO** special capabilities:

- ❌ CAP_NET_ADMIN (no network configuration)
- ❌ CAP_SYS_ADMIN (no system administration)
- ❌ CAP_DAC_OVERRIDE (respects file permissions)
- ❌ CAP_SETUID / CAP_SETGID (no user switching)
- ✅ Standard file read/write only

## Security Audit Notes

**Privilege Boundary**: All operations run in userspace as unprivileged `glyphos` user.

**Attack Surface**: Limited to vault directory and log files. No network exposure, no privilege escalation paths.

**Recommended Hardening**:
1. SELinux/AppArmor policy (restrict file access to vault only)
2. Readonly filesystem for binaries
3. Audit logging for vault access
4. File integrity monitoring (AIDE, Tripwire)

## Sign-Off

Once validation is complete, document the results:

```
Privilege Model Validation Completed: [DATE]
Validated by: [NAME]
Environment: [FreeBSD 13.2 / FreeBSD 14.0 / Linux]

Results:
- [ ] All checklist items passed
- [ ] Service runs as non-root: YES / NO
- [ ] No elevated capabilities: YES / NO
- [ ] Functionality intact: YES / NO

Issues found: [NONE / LIST]

Approved for production: YES / NO
Signature: _______________
```
