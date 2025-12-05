# Claude ISO Production Handoff - Complete Bundle

**Status**: ✅ **ALL COMPONENTS READY**
**Date**: 2025-12-05T18:30:00Z
**Branch**: `feature/release-readiness`
**Repository**: https://github.com/EarthwebAP/glyph-os-prototype

---

## 🎯 Quick Start for Claude

**Primary Document**: `docs/ISO_PRODUCTION_BUNDLE.md`

**Execute these commands in order** (copy-paste ready):
```bash
# See "Exact Commands for Claude" section below
```

**Expected Timeline**: 2-4 hours → Signed ISO ready

---

## 📦 Files and Repository State

### Repository Access
- **URL**: https://github.com/EarthwebAP/glyph-os-prototype
- **Branch**: `feature/release-readiness`
- **Latest Commit**: `4fff029` - "feat: add ISO smoke test and structured fuzz reporting"

### Primary Documentation ✅
- **docs/ISO_PRODUCTION_BUNDLE.md** - Master execution guide (737 lines)
  - 10-step process with exact commands
  - Acceptance criteria (7 items)
  - Troubleshooting guide
  - PR comment template

### Build Scripts ✅
- **scripts/unified_pipeline.sh** - Unified build pipeline
- **ci/generate_release_manifest.sh** - Release manifest generator
- **ci/generate_iso.sh** - ISO generation wrapper (334 lines)
- **scripts/sign_artifacts.sh** - Multi-method signing (422 lines)
- **build_iso.sh** - Production FreeBSD ISO builder

### Determinism Tools ✅
- **ci/determinism_check.sh** - Build determinism verifier
- **ci/toolchain.lock** - Reproducible build lockfile
  - gcc 13.3.0, binutils 2.42
  - SOURCE_DATE_EPOCH=1701820800
  - All env vars documented

### Signing & Verification ✅
- **scripts/sign_artifacts.sh** - GPG, Cosign, KMS signing
- **scripts/verify_proof.sh** - RSA proof verifier (shell)
- **scripts/verify_proof.py** - RSA proof verifier (Python)

### Testing & Validation ✅
- **scripts/iso_smoke_test.sh** - ISO smoke test (380 lines)
  - Format validation, checksum, boot test
  - Optional QEMU and mount tests
  - Generates logs/iso_smoke.log
- **ci/security_tests.sh** - Security regression tests

### CI Workflows ✅
- **.github/workflows/ci.yml** - Main CI pipeline
  - 7 jobs: build-and-test, sanitizers, determinism-check, security-scan, sign-artifacts, fuzzing, summary
  - Includes artifact signing job
- **.github/workflows/nightly-canonical.yml** - Nightly determinism verification
  - Daily 2 AM UTC builds
  - Parity checking
  - Auto GitHub Releases

### Artifact Collection ✅
- **scripts/collect_ci_artifacts.sh** - CI artifact collector
- **scripts/package_auditor_bundle.sh** - Security audit bundler
- **scripts/provision_github_secrets.sh** - Secrets provisioning

### Current Artifacts ✅
- **artifacts/checksums.sha256** - Binary checksums
- **artifacts/substrate_core** (21K)
- **artifacts/glyph_interp** (30K)
- **ci/fuzz_report.json** - Structured fuzz results
  - 10,000 executions, 0 crashes
  - 84.46% line coverage
  - All acceptance criteria met
- **ci/fuzz_results.txt** - Plain text fuzz results
- **logs/backup_test.log** - Backup test PASSED

### Documentation ✅
- **docs/STAGING_HARDWARE.md** - Hardware specs and soak test plan
- **docs/PHASE_ASSIGNMENTS.md** - Phase 4-8 with owner contacts
- **docs/FINAL_15_PERCENT_ROADMAP.md** - Remaining work (15-20%)
- **docs/PHASE4_KICKOFF_AGENDA.md** - Kickoff meeting plan
- **.github/PULL_REQUEST_TEMPLATE_PHASE4_FINAL.md** - PR template

### Directories Created ✅
- **artifacts/** - Binary artifacts and checksums
- **dist/** - ISO output (gitignored, created at build time)
- **logs/sanitizers/** - Sanitizer outputs
- **ci/fuzz_crashes/** - Crash reproducers (empty = no crashes)

---

## 🔐 Secrets and Access (Provision Before Claude Runs)

### GitHub Repository Secrets

Add these to repository settings → Secrets and variables → Actions:

```bash
# Required for artifact signing
GPG_PRIVATE_KEY=<armored or base64 GPG private key>
GPG_PASSPHRASE=<passphrase if key is encrypted>

# Optional: Cosign keyless (uses GitHub OIDC, no secret needed)
COSIGN_EXPERIMENTAL=1

# Optional: Cloud KMS signing
# For AWS KMS:
AWS_ACCESS_KEY_ID=<aws_key_id>
AWS_SECRET_ACCESS_KEY=<aws_secret>
AWS_KMS_KEY_ID=<kms_key_id>

# For GCP KMS:
GCP_SA_KEY=<service_account_json>
GCP_KMS_KEY_ID=<kms_key_id>

# Optional: Self-hosted runner registration
RUNNER_REG_TOKEN=<temporary_token>
```

**⚠️ CRITICAL**: DO NOT commit private keys to repository. Use secrets or KMS/HSM only.

### Runner Labels to Provide

If using self-hosted runners, configure these labels:
- `self-hosted`
- `iso-builder`
- `sanitizer`
- `fuzzer`
- `gpu` (for GPU nodes)
- `fpga-sim` (for FPGA nodes)

### Permissions Required

Grant Claude automation account:
- ✅ Push access to `feature/release-readiness`
- ✅ Permission to create GitHub Releases
- ✅ Permission to upload artifacts
- ✅ Access to repository secrets (for CI jobs)
- ✅ Runner access (if using self-hosted)

### Network Access

Ensure runners can reach:
- ✅ GitHub API and artifact storage
- ✅ KMS/HSM endpoints (if using cloud signing)
- ✅ FreeBSD package mirrors (for ISO build)
- ✅ S3-compatible artifact storage (for corpus retention)

---

## 🖥️ Runners and Staging Nodes (Reserve Now)

### Required Immediately

**ISO Builder** (CRITICAL - required for ISO generation):
- **CPU**: 8 vCPU
- **RAM**: 32 GB
- **Storage**: 200 GB SSD
- **OS**: FreeBSD 14.0-RELEASE
- **Network**: 100 Mbps (for FreeBSD base download)
- **Label**: `iso-builder`

### Required by Phase 6 (Jan 6, 2026)

**Soak Test Nodes** (3 nodes):

1. **CPU Node**:
   - 8 vCPU, 32 GB RAM
   - Ubuntu 24.04 or FreeBSD 14.0
   - Label: `soak-cpu`

2. **GPU Node**:
   - 8 vCPU, 64 GB RAM, NVIDIA GPU
   - CUDA drivers installed
   - Label: `soak-gpu`

3. **FPGA Sim Node**:
   - 16 vCPU, 128 GB RAM
   - FPGA dev board access or simulator
   - Label: `soak-fpga`

**Fuzzer Runners** (2 nodes, for Phase 5):
- **CPU**: 16 vCPU each
- **RAM**: 64 GB each
- **Storage**: 1 TB NVMe (persistent corpus)
- **Label**: `fuzzer`

**Monitoring Server**:
- **CPU**: 4 vCPU
- **RAM**: 16 GB
- **Services**: Prometheus + Grafana
- **Access**: To all staging node metrics (ports 9102, 9103)

**Artifact Storage**:
- S3-compatible or NFS
- 1 TB retention for corpora and crash artifacts
- 90-day retention for build artifacts

---

## 🚀 Exact Commands for Claude to Run

Execute these commands **in order** on the ISO builder or CI runner.

### Step 1: Prepare Deterministic Environment

```bash
export TZ=UTC
export LANG=C
export LC_ALL=C
export SOURCE_DATE_EPOCH=1701820800
export GDF_SEED=0

# Verify environment
echo "TZ=$TZ LANG=$LANG SOURCE_DATE_EPOCH=$SOURCE_DATE_EPOCH GDF_SEED=$GDF_SEED"
```

**Expected Output**:
```
TZ=UTC LANG=C SOURCE_DATE_EPOCH=1701820800 GDF_SEED=0
```

---

### Step 2: Clone Repository and Checkout Branch

```bash
git clone https://github.com/EarthwebAP/glyph-os-prototype.git
cd glyph-os-prototype/freebsd
git checkout feature/release-readiness

# Verify checkout
git log --oneline -1
```

**Expected Output**:
```
4fff029 feat: add ISO smoke test and structured fuzz reporting
```

---

### Step 3: Clean Build and Run Tests

```bash
./scripts/unified_pipeline.sh --clean --ci | tee logs/build_run.log

# Check exit code
if [ $? -eq 0 ]; then
    echo "✓ Build successful"
else
    echo "✗ Build failed - check logs/build_run.log"
    exit 1
fi
```

**Expected Output**:
```
✓ Build successful
Build completed in X seconds
Tests: 16/16 passed
```

---

### Step 4: Generate Manifest and Checksums

```bash
# Generate release manifest
sh ci/generate_release_manifest.sh > release_manifest_generated.json

# Verify manifest is valid JSON
jq '.' release_manifest_generated.json >/dev/null && echo "✓ Valid manifest"

# Generate checksums
mkdir -p artifacts
sha256sum bin/* > artifacts/checksums.sha256

# Display checksums
cat artifacts/checksums.sha256
```

**Expected Output**:
```
✓ Valid manifest
d9bc1bd2a5f434f968137d77fc64dd480df45f4e474ed2154026df428baf20de  bin/substrate_core
132a7c79f7d89da8f88bccc0f57727ad4ee10cee940b9a09dab5c09a75871e7b  bin/glyph_interp
```

---

### Step 5: Sign Artifacts

**Option A: Local GPG signing (dry-run)**

```bash
# Import GPG key if needed
# gpg --import <key_file>

# Sign checksums
gpg --batch --yes --armor \
    --output artifacts/checksums.sha256.asc \
    --detach-sign artifacts/checksums.sha256

# Verify signature
gpg --verify artifacts/checksums.sha256.asc artifacts/checksums.sha256
```

**Option B: Use signing script (recommended)**

```bash
# Copy binaries to artifacts
cp bin/* artifacts/

# Run signing script
./scripts/sign_artifacts.sh --gpg --artifacts artifacts/

# Verify signatures created
ls -lh artifacts/*.asc
```

**Expected Output**:
```
gpg: Good signature from "GlyphOS Build <...>"
-rw-r--r-- 1 user user 833 Dec  5 18:00 checksums.sha256.asc
```

---

### Step 6: Run Determinism Parity Check

```bash
sh ci/determinism_check.sh | tee logs/determinism.log

# Check result
if grep -q "IDENTICAL" logs/determinism.log; then
    echo "✓ Determinism verified"
else
    echo "✗ Non-deterministic build detected"
    cat logs/determinism.log
    exit 1
fi
```

**Expected Output**:
```
Building run 1...
Building run 2...
Comparing checksums...
✓ IDENTICAL
All builds are deterministic.
```

---

### Step 7: Build ISO

```bash
# Run ISO generation script
sh ci/generate_iso.sh --smoke-test --sign | tee logs/iso_build.log

# Check for ISO
if [ -f dist/glyphos-v0.1.0-alpha.iso ]; then
    echo "✓ ISO built successfully"
    ls -lh dist/glyphos-v0.1.0-alpha.iso
else
    echo "✗ ISO build failed - check logs/iso_build.log"
    exit 1
fi
```

**Expected Output**:
```
✓ ISO built successfully
-rw-r--r-- 1 user user 800M Dec  5 18:30 dist/glyphos-v0.1.0-alpha.iso
```

---

### Step 8: Smoke Test ISO on Staging VM

```bash
# Run smoke test
sh scripts/iso_smoke_test.sh dist/glyphos-v0.1.0-alpha.iso | tee logs/iso_smoke.log

# Optional: Boot test with QEMU
# sh scripts/iso_smoke_test.sh dist/glyphos-v0.1.0-alpha.iso --boot-test

# Check result
if grep -q "PASSED" logs/iso_smoke.log; then
    echo "✓ Smoke test passed"
else
    echo "✗ Smoke test failed"
    cat logs/iso_smoke.log
    exit 1
fi
```

**Expected Output**:
```
✓ Smoke test PASSED
Result: PASSED
```

---

### Step 9: Verify Signatures and Proofs

```bash
# Verify ISO checksum
sha256sum -c dist/glyphos-v0.1.0-alpha.iso.sha256

# Verify GPG signature
gpg --verify dist/glyphos-v0.1.0-alpha.iso.sha256.asc dist/glyphos-v0.1.0-alpha.iso.sha256

# Verify cryptographic proof (if exists)
if [ -f benchmarks/dma_roundtrip.json ]; then
    sh scripts/verify_proof.sh benchmarks/dma_roundtrip.json | tee logs/proof_verify.log
fi
```

**Expected Output**:
```
dist/glyphos-v0.1.0-alpha.iso: OK
gpg: Good signature from "GlyphOS Build <...>"
✓ PROOF OK
```

---

### Step 10: Package Auditor Bundle and Upload

```bash
# Generate auditor bundle
sh scripts/package_auditor_bundle.sh

# Verify bundle created
if [ -f artifacts/auditor_bundle.tar.gz ]; then
    echo "✓ Audit bundle created"
    tar -tzf artifacts/auditor_bundle.tar.gz | head -10
else
    echo "✗ Audit bundle generation failed"
    exit 1
fi

# Collect all CI artifacts
./scripts/collect_ci_artifacts.sh feature/release-readiness

# Create GitHub Release
gh release create v0.1.0-alpha \
    --title "GlyphOS Alpha Release v0.1.0" \
    --notes "Production-ready alpha release.

**Checksums**:
\`\`\`
$(cat artifacts/checksums.sha256)
\`\`\`

**ISO**:
\`\`\`
$(cat dist/glyphos-v0.1.0-alpha.iso.sha256)
\`\`\`

**Verification**:
\`\`\`bash
sha256sum -c glyphos-v0.1.0-alpha.iso.sha256
gpg --verify checksums.sha256.asc
\`\`\`
" \
    dist/glyphos-v0.1.0-alpha.iso \
    dist/glyphos-v0.1.0-alpha.iso.sha256 \
    artifacts/checksums.sha256 \
    artifacts/checksums.sha256.asc \
    release_manifest_generated.json \
    artifacts/auditor_bundle.tar.gz \
    --prerelease
```

**Expected Output**:
```
✓ Audit bundle created
https://github.com/EarthwebAP/glyph-os-prototype/releases/tag/v0.1.0-alpha
```

---

## ✅ Acceptance Criteria (Claude Must Meet All 7)

Before posting the PR comment, verify ALL criteria:

### 1. Signed ISO and Checksum

```bash
# Check files exist
test -f dist/glyphos-v0.1.0-alpha.iso && echo "✓ ISO exists"
test -f dist/glyphos-v0.1.0-alpha.iso.sha256 && echo "✓ Checksum exists"
test -f dist/glyphos-v0.1.0-alpha.iso.sha256.asc && echo "✓ Signature exists"
```

### 2. Determinism Proof

```bash
# Verify determinism log shows IDENTICAL for 3 builds or parity
grep -q "IDENTICAL" logs/determinism.log && echo "✓ Determinism verified"
cat logs/determinism.log
```

### 3. Sanitizer Status

```bash
# Check for sanitizer errors
if grep -r "ERROR: AddressSanitizer\|ERROR: UndefinedBehaviorSanitizer" logs/ 2>/dev/null; then
    echo "✗ Sanitizer errors found"
else
    echo "✓ Sanitizers clean (0 errors)"
fi
```

### 4. Fuzz Baseline

```bash
# Check fuzz report
jq '.crashes | length' ci/fuzz_report.json
# Expected: 0

jq '.acceptance_criteria.zero_critical_crashes.met' ci/fuzz_report.json
# Expected: true
```

### 5. Smoke Test

```bash
# Verify smoke test passed
grep -q "PASSED" logs/iso_smoke.log && echo "✓ Smoke test passed"
cat logs/iso_smoke.log
```

### 6. Release Manifest

```bash
# Verify manifest is valid JSON
jq '.' release_manifest_generated.json >/dev/null && echo "✓ Valid manifest"

# Show manifest summary
jq '{version, build_date, components: .components | map({name, sha256})}' release_manifest_generated.json
```

### 7. Audit Bundle

```bash
# Verify audit bundle
test -f artifacts/auditor_bundle.tar.gz && echo "✓ Audit bundle exists"

# Show contents summary
tar -tzf artifacts/auditor_bundle.tar.gz | wc -l
echo "files in bundle"
```

---

## 📝 PR Comment (Claude Posts When Ready)

Once all acceptance criteria are met, Claude posts this comment to the PR:

```markdown
## ✅ ISO Ready for Review

**ISO**: `dist/glyphos-v0.1.0-alpha.iso` (800MB)
**Branch**: `feature/release-readiness`
**Build Date**: 2025-12-05T18:30:00Z
**Commit**: 4fff029

### Artifacts

- **ISO**: dist/glyphos-v0.1.0-alpha.iso
- **ISO Checksum**: dist/glyphos-v0.1.0-alpha.iso.sha256
- **ISO Signature**: dist/glyphos-v0.1.0-alpha.iso.sha256.asc
- **Binary Checksums**: artifacts/checksums.sha256
- **GPG Signature**: artifacts/checksums.sha256.asc
- **Release Manifest**: release_manifest_generated.json
- **Audit Bundle**: artifacts/auditor_bundle.tar.gz

### Verification Logs

- **Determinism**: logs/determinism.log → ✅ IDENTICAL
- **Build Log**: logs/build_run.log → ✅ PASSED (16/16 tests)
- **Smoke Test**: logs/iso_smoke.log → ✅ PASSED
- **Sanitizers**: logs/*_san.log → ✅ CLEAN (0 errors)
- **Fuzz Report**: ci/fuzz_report.json → ✅ 0 crashes, 84.46% coverage

### Checksums

**Binaries**:
```
d9bc1bd2a5f434f968137d77fc64dd480df45f4e474ed2154026df428baf20de  bin/substrate_core
132a7c79f7d89da8f88bccc0f57727ad4ee10cee940b9a09dab5c09a75871e7b  bin/glyph_interp
```

**ISO**:
```
<actual_iso_sha256>  dist/glyphos-v0.1.0-alpha.iso
```

### Acceptance Criteria

- [x] **Signed ISO and checksum** - GPG signature present
- [x] **Determinism proof** - IDENTICAL builds verified
- [x] **Sanitizer status** - ASan/UBSan clean (0 errors)
- [x] **Fuzz baseline** - 0 critical crashes, 10,000 executions
- [x] **Smoke test** - All checks passed
- [x] **Release manifest** - Valid JSON with all components
- [x] **Audit bundle** - Complete with threat model and logs

### Next Steps

1. Download ISO from GitHub Release
2. Verify checksum: `sha256sum -c dist/glyphos-v0.1.0-alpha.iso.sha256`
3. Verify signature: `gpg --verify dist/glyphos-v0.1.0-alpha.iso.sha256.asc`
4. Deploy to staging hardware for soak testing
5. Run 72-hour stability test (Phase 6)

**Ready for deployment** ✅
```

---

## 🔍 Quick Verification Steps (After Claude Posts)

Run these commands locally to verify Claude's deliverables:

### 1. Checksum and Signature

```bash
# Download artifacts from GitHub Release
gh release download v0.1.0-alpha

# Verify ISO checksum
sha256sum -c glyphos-v0.1.0-alpha.iso.sha256

# Verify GPG signature
gpg --verify glyphos-v0.1.0-alpha.iso.sha256.asc glyphos-v0.1.0-alpha.iso.sha256
```

**Expected**: Both verification steps pass with "OK" and "Good signature"

### 2. Run Proof Verifier

```bash
# If proof file exists
sh scripts/verify_proof.sh benchmarks/dma_roundtrip.json
```

**Expected**: `✓ PROOF OK`

### 3. Boot Smoke Test Locally

```bash
# Quick smoke test
sh scripts/iso_smoke_test.sh glyphos-v0.1.0-alpha.iso --quick

# Full smoke test with boot
sh scripts/iso_smoke_test.sh glyphos-v0.1.0-alpha.iso --boot-test
```

**Expected**: `Result: PASSED`

---

## ⚠️ Urgent Cautions

### 1. DO NOT Commit Private Keys
- **NEVER** commit GPG private keys to repository
- Use GitHub Secrets: `scripts/provision_github_secrets.sh`
- For local testing, use test keys in `ci/keys/test_privkey.pem`
- Rotate keys after use if exposed

### 2. Runner Registration Tokens
- If Claude needs temporary runner registration tokens, **rotate them after use**
- Use short-lived tokens (max 24 hours)
- Revoke immediately after runner setup

### 3. Faster ISO Without Full Audit (Risk Acceptance)
If you need the ISO immediately:
- ✅ Claude can produce functional ISO now
- ⚠️ **RISK**: ISO not fully hardened
- ✅ Run extended fuzzing/soak (Phase 5-6) in parallel
- ⚠️ Be prepared to **reissue ISO** if critical findings emerge

### 4. Toolchain Lock Authority
- `ci/toolchain.lock` is authoritative for reproducible builds
- If updating toolchain, regenerate lock and re-verify determinism
- Document CVE justification for emergency updates

---

## 📚 Additional Resources

### Documentation
- **Primary**: `docs/ISO_PRODUCTION_BUNDLE.md` (start here)
- Phase Assignments: `docs/PHASE_ASSIGNMENTS.md`
- Final Roadmap: `docs/FINAL_15_PERCENT_ROADMAP.md`
- Security Patches: `docs/SECURITY_PATCHES.md`
- Monitoring: `docs/MONITORING.md`
- Operations: `docs/OPERATIONALIZATION.md`
- Staging Hardware: `docs/STAGING_HARDWARE.md`

### Workflows
- Main CI: `.github/workflows/ci.yml`
- Nightly Canonical: `.github/workflows/nightly-canonical.yml`

### Support
- **Build Engineer** (Phase 4 owner): See `docs/PHASE_ASSIGNMENTS.md`
- **DevOps Lead** (CI automation): See `docs/PHASE_ASSIGNMENTS.md`
- **Security Engineer** (signing): See `docs/PHASE_ASSIGNMENTS.md`

### Issue Tracking
- GitHub Issues: https://github.com/EarthwebAP/glyph-os-prototype/issues
- Labels: `phase-4-determinism`, `iso-production`

---

## 🎁 Final Handoff

**Give Claude**:
1. This document (`CLAUDE_HANDOFF_COMPLETE.md`)
2. Repository access (branch: `feature/release-readiness`)
3. GitHub secrets (provisioned via `scripts/provision_github_secrets.sh`)
4. Runner access with correct labels
5. ISO builder node (FreeBSD 14.0, 8 vCPU, 32 GB RAM)

**Claude Executes**:
1. Reads `docs/ISO_PRODUCTION_BUNDLE.md`
2. Runs exact commands (Steps 1-10 above)
3. Verifies all 7 acceptance criteria
4. Posts PR comment with artifact links
5. Uploads to GitHub Release

**You Verify**:
1. Download artifacts from GitHub Release
2. Run verification steps above (checksums, signatures, smoke test)
3. Confirm all acceptance criteria met
4. Deploy to staging for Phase 6 soak testing

**Timeline**: 2-4 hours → **PRODUCTION ISO READY** ✅

---

**Status**: 🎯 **COMPLETE AND READY**
**Confidence**: ✅ **HIGH**
**Risk**: ✅ **LOW**
**Next Action**: Hand to Claude, execute, verify.
