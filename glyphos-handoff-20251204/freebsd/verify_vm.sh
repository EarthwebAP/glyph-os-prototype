#!/bin/sh
#
# GlyphOS VM Verification Script
# Run this INSIDE the booted GlyphOS VM to verify all components
#
# Usage: ./verify_vm.sh

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

PASS=0
FAIL=0

check_pass() {
    echo "${GREEN}✓${NC} $1"
    PASS=$((PASS + 1))
}

check_fail() {
    echo "${RED}✗${NC} $1"
    FAIL=$((FAIL + 1))
}

check_warn() {
    echo "${YELLOW}⚠${NC} $1"
}

echo "======================================="
echo "GlyphOS VM Verification"
echo "======================================="
echo

# 1. OS Version
echo "[1/12] Checking OS version..."
if uname -sr | grep -q "FreeBSD"; then
    check_pass "Running on FreeBSD $(uname -r)"
else
    check_fail "Not running on FreeBSD"
fi
echo

# 2. glyphd Service
echo "[2/12] Checking glyphd service..."
if service glyphd status >/dev/null 2>&1; then
    check_pass "glyphd service is running"
    if curl -sf http://localhost:8080/health >/dev/null 2>&1; then
        check_pass "glyphd health endpoint responds"
    else
        check_fail "glyphd health endpoint not responding"
    fi
else
    check_fail "glyphd service not running"
fi
echo

# 3. glyph-spu Service
echo "[3/12] Checking glyph-spu service..."
if service glyph_spu status >/dev/null 2>&1; then
    check_pass "glyph-spu service is running"
    if curl -sf http://localhost:8081/health >/dev/null 2>&1; then
        check_pass "glyph-spu health endpoint responds"
    else
        check_fail "glyph-spu health endpoint not responding"
    fi
else
    check_fail "glyph-spu service not running"
fi
echo

# 4. pf Firewall
echo "[4/12] Checking pf firewall..."
if service pf status >/dev/null 2>&1; then
    check_pass "pf firewall is running"
    RULES=$(pfctl -sr 2>/dev/null | wc -l)
    if [ "$RULES" -gt 0 ]; then
        check_pass "pf has $RULES rules loaded"
    else
        check_fail "pf has no rules loaded"
    fi
else
    check_fail "pf firewall not running"
fi
echo

# 5. SSH Service
echo "[5/12] Checking SSH service..."
if service sshd status >/dev/null 2>&1; then
    check_pass "sshd service is running"
    if grep -q "PermitRootLogin no" /etc/ssh/sshd_config; then
        check_pass "Root login disabled"
    else
        check_fail "Root login not disabled"
    fi
    if grep -q "PasswordAuthentication no" /etc/ssh/sshd_config; then
        check_pass "Password authentication disabled"
    else
        check_warn "Password authentication not disabled"
    fi
else
    check_fail "sshd not running"
fi
echo

# 6. Monitoring Exporters
echo "[6/12] Checking monitoring exporters..."
if ps aux | grep -v grep | grep -q node_exporter; then
    check_pass "node_exporter is running"
    if curl -sf http://localhost:9100/metrics >/dev/null 2>&1; then
        check_pass "node_exporter metrics endpoint responds"
    else
        check_fail "node_exporter endpoint not responding"
    fi
else
    check_warn "node_exporter not running"
fi

if ps aux | grep -v grep | grep -q glyphd_exporter; then
    check_pass "glyphd_exporter is running"
    if curl -sf http://localhost:9101/metrics >/dev/null 2>&1; then
        check_pass "glyphd_exporter metrics endpoint responds"
    else
        check_fail "glyphd_exporter endpoint not responding"
    fi
else
    check_warn "glyphd_exporter not running"
fi
echo

# 7. Persistence Layer
echo "[7/12] Checking persistence layer..."
if [ -d "/usr/local/glyphos/data" ]; then
    check_pass "Persistence directory exists"
    PERMS=$(stat -f "%Op" /usr/local/glyphos/data 2>/dev/null | tail -c 4)
    if [ "$PERMS" = "700" ]; then
        check_pass "Persistence directory has correct permissions (700)"
    else
        check_warn "Persistence directory permissions: $PERMS (expected 700)"
    fi
else
    check_fail "Persistence directory not found"
fi
echo

# 8. Security Hardening (sysctl)
echo "[8/12] Checking sysctl hardening..."
if [ "$(sysctl -n net.inet.tcp.syncookies 2>/dev/null)" = "1" ]; then
    check_pass "SYN cookies enabled"
else
    check_fail "SYN cookies not enabled"
fi

if [ "$(sysctl -n kern.securelevel 2>/dev/null)" -ge "1" ]; then
    check_pass "Securelevel is $(sysctl -n kern.securelevel)"
else
    check_warn "Securelevel is $(sysctl -n kern.securelevel) (expected >= 1)"
fi

if [ "$(sysctl -n security.bsd.see_other_uids 2>/dev/null)" = "0" ]; then
    check_pass "Process isolation enabled (see_other_uids=0)"
else
    check_warn "Process isolation not enabled"
fi
echo

# 9. Audit Daemon
echo "[9/12] Checking audit daemon..."
if service auditd status >/dev/null 2>&1; then
    check_pass "auditd service is running"
    if [ -d "/var/audit" ]; then
        AUDIT_FILES=$(ls /var/audit/ 2>/dev/null | wc -l)
        if [ "$AUDIT_FILES" -gt 0 ]; then
            check_pass "Audit logs present ($AUDIT_FILES files)"
        else
            check_warn "No audit log files yet"
        fi
    else
        check_fail "Audit directory not found"
    fi
else
    check_warn "auditd not running"
fi
echo

# 10. Network Configuration
echo "[10/12] Checking network configuration..."
if ifconfig | grep -q "inet "; then
    IP=$(ifconfig | grep "inet " | grep -v "127.0.0.1" | head -1 | awk '{print $2}')
    check_pass "Network configured (IP: $IP)"
else
    check_fail "No network configuration found"
fi

if ping -c 1 8.8.8.8 >/dev/null 2>&1; then
    check_pass "Internet connectivity (ping 8.8.8.8)"
else
    check_warn "No internet connectivity"
fi
echo

# 11. Users
echo "[11/12] Checking users..."
if id glyphd >/dev/null 2>&1; then
    check_pass "glyphd user exists"
    GLYPHD_UID=$(id -u glyphd)
    check_pass "glyphd UID: $GLYPHD_UID"
else
    check_fail "glyphd user not found"
fi
echo

# 12. Update Mechanism
echo "[12/12] Checking update mechanism..."
if [ -f "/usr/local/sbin/glyphos-update" ]; then
    check_pass "glyphos-update script exists"
    if [ -x "/usr/local/sbin/glyphos-update" ]; then
        check_pass "glyphos-update is executable"
    else
        check_fail "glyphos-update not executable"
    fi
else
    check_fail "glyphos-update script not found"
fi

if [ -f "/etc/freebsd-update.conf" ]; then
    check_pass "freebsd-update.conf exists"
else
    check_fail "freebsd-update.conf not found"
fi
echo

# Summary
echo "======================================="
echo "Verification Summary"
echo "======================================="
echo "${GREEN}Passed:${NC} $PASS"
echo "${RED}Failed:${NC} $FAIL"
echo

if [ $FAIL -eq 0 ]; then
    echo "${GREEN}✓ All critical checks passed!${NC}"
    echo "GlyphOS node is production-ready."
    exit 0
else
    echo "${RED}✗ Some checks failed${NC}"
    echo "Review the output above for details."
    exit 1
fi
