# Phase 4: Determinism Hardening - Pull Request

## Overview

**Phase**: 4 of 8 (Determinism Hardening)
**Duration**: 1-2 weeks
**Status**: 🚀 Ready for Review
**Tracking**: Phase 4 Kickoff - Dec 9, 2025

**Goal**: Achieve bit-identical reproducible builds for supply chain security and third-party verification.

---

## Checklist

### Required Artifacts ✅

Upload and link all required artifacts:

- [ ] **Toolchain Lockfile**: `ci/toolchain.lock` committed
  - Contains: Package versions, compiler checksums, env vars
  - Verified: Lockfile produces identical builds locally

- [ ] **Canonical Build Manifest**: `canonical-manifest.json`
  - Build date, git commit, binary SHA256 checksums
  - Attached to PR or uploaded to artifacts/

- [ ] **Checksums**: `artifacts/checksums.sha256` and signature
  - SHA256 for substrate_core, glyph_interp
  - GPG or Cosign signature attached

- [ ] **Determinism Logs**: `logs/determinism.log`
  - Three consecutive nightly builds with identical checksums
  - Diff output showing 0 differences

- [ ] **Sanitizer Logs**: `logs/*_san.log`
  - AddressSanitizer: 0 errors
  - UndefinedBehaviorSanitizer: 0 errors
  - MemorySanitizer: 0 errors

- [ ] **CI Workflow**: `.github/workflows/nightly-canonical.yml`
  - Nightly build job configured
  - Determinism comparison implemented
  - Manual trigger tested

---

### Code Changes

- [ ] **Toolchain Lock Script**: `ci/generate_toolchain_lock.sh` created
- [ ] **Toolchain Lockfile**: `ci/toolchain.lock` generated and committed
- [ ] **Nightly Workflow**: `.github/workflows/nightly-canonical.yml` added
- [ ] **CI Updates**: Reproducibility env vars enforced in all jobs
- [ ] **Determinism Gate**: CI fails on non-deterministic builds
- [ ] **Documentation**: `docs/REPRODUCIBLE_BUILDS.md` created

---

### Testing

- [ ] **Local Build Test**: Built twice locally, checksums match
  ```bash
  # Build 1
  make clean && make
  sha256sum bin/substrate_core > build1.sha256

  # Build 2
  make clean && make
  sha256sum bin/substrate_core > build2.sha256

  # Compare
  diff build1.sha256 build2.sha256  # Should be identical
  ```

- [ ] **Nightly Build Test**: Three consecutive nightly runs passed
  - Run 1: [Link to GitHub Actions run]
  - Run 2: [Link to GitHub Actions run]
  - Run 3: [Link to GitHub Actions run]
  - Checksums: All identical ✅

- [ ] **Toolchain Validation**: Lockfile rebuilds identical environment
  ```bash
  # Install from lockfile
  ci/install_from_lockfile.sh

  # Build
  make

  # Verify matches canonical
  diff checksums.sha256 canonical-checksums.sha256
  ```

- [ ] **Sanitizer Re-run**: All sanitizers clean after changes
  - AddressSanitizer: PASS
  - UndefinedBehaviorSanitizer: PASS
  - MemorySanitizer: PASS

---

### Acceptance Criteria (Phase 4 Exit Gates)

All criteria must be met:

- [ ] ✅ **Three consecutive nightly builds** produce identical SHA256 checksums
  - substrate_core: `[CHECKSUM]`
  - glyph_interp: `[CHECKSUM]`
  - Variance: 0 bytes

- [ ] ✅ **Toolchain lockfile** committed and enforced by CI
  - File: `ci/toolchain.lock`
  - Validation: CI installs exact versions from lockfile

- [ ] ✅ **Reproducibility env vars** present in all CI jobs
  - SOURCE_DATE_EPOCH=1701820800
  - TZ=UTC
  - LANG=C
  - LC_ALL=C
  - GDF_SEED=0

- [ ] ✅ **Nightly parity check** running and passing
  - Workflow: `.github/workflows/nightly-canonical.yml`
  - Schedule: Daily at 2 AM UTC
  - Last run: PASS

- [ ] ✅ **CI determinism gate** blocks non-deterministic builds
  - Non-deterministic builds fail CI
  - Clear error message pointing to determinism logs
  - Tested with intentionally non-deterministic build

- [ ] ✅ **No new sanitizer warnings** introduced
  - ASan: 0 errors
  - UBSan: 0 errors
  - MSan: 0 errors

- [ ] ✅ **Documentation complete**
  - `docs/REPRODUCIBLE_BUILDS.md` created
  - Toolchain lockfile usage documented
  - Determinism troubleshooting guide included

---

## Test Results

### Determinism Verification

```bash
# Three consecutive nightly builds:

Build 1 (Dec 17, 2025):
substrate_core: a1b2c3d4e5f6...
glyph_interp:   f6e5d4c3b2a1...

Build 2 (Dec 18, 2025):
substrate_core: a1b2c3d4e5f6...  ✅ MATCH
glyph_interp:   f6e5d4c3b2a1...  ✅ MATCH

Build 3 (Dec 19, 2025):
substrate_core: a1b2c3d4e5f6...  ✅ MATCH
glyph_interp:   f6e5d4c3b2a1...  ✅ MATCH

Determinism: 100% (3/3 builds identical)
```

### Sanitizer Results

```
AddressSanitizer: 0 errors, 0 warnings
UndefinedBehaviorSanitizer: 0 errors, 0 warnings
MemorySanitizer: 0 errors, 0 warnings

All tests PASSED
```

### Performance Impact

```
Build time variance: 2.3% (within acceptable range < 5%)
Binary size: No change
Memory usage: No change
```

---

## Risk Assessment

### Risks Mitigated ✅

- ✅ **Toolchain drift**: Lockfile prevents unintentional version changes
- ✅ **Timestamp leakage**: SOURCE_DATE_EPOCH prevents build timestamps
- ✅ **Non-determinism regression**: CI gate catches issues immediately

### Remaining Risks ⚠️

- ⚠️ **Toolchain availability**: Locked versions may become unavailable
  - Mitigation: Mirror packages in internal repository

- ⚠️ **Emergency updates**: Critical CVE may require toolchain update
  - Mitigation: Fast-track lockfile update, re-run 3 nightly builds

---

## Rollout Plan

### Phase 4 Completion (Week of Dec 16-20)

**Monday Dec 16**:
- [ ] CI determinism gate implemented
- [ ] First nightly build with gate enabled

**Tuesday Dec 17**:
- [ ] Second consecutive nightly build
- [ ] Review determinism logs

**Wednesday Dec 18**:
- [ ] Third consecutive nightly build
- [ ] Verify all three builds identical

**Thursday Dec 19**:
- [ ] Phase 4 acceptance criteria review
- [ ] Sign-off from all stakeholders

**Friday Dec 20**:
- [ ] Phase 4 retrospective
- [ ] Merge to main
- [ ] Handoff to Phase 5 (Fuzzing)

---

## Sign-off

**Required Approvals**:

- [ ] **Build Engineer**: Phase 4 acceptance criteria met
  - Name: [NAME]
  - Date: [DATE]
  - Signature: [GITHUB_USERNAME]

- [ ] **DevOps Lead**: CI determinism gate operational and tested
  - Name: [NAME]
  - Date: [DATE]
  - Signature: [GITHUB_USERNAME]

- [ ] **Security Engineer**: Toolchain lockfile reviewed and approved
  - Name: [NAME]
  - Date: [DATE]
  - Signature: [GITHUB_USERNAME]

- [ ] **Project Manager**: Timeline and deliverables complete
  - Name: [NAME]
  - Date: [DATE]
  - Signature: [GITHUB_USERNAME]

---

## Related Issues

- Closes: #[ISSUE_NUMBER] - Phase 4: Determinism Hardening
- Blocks: #[ISSUE_NUMBER] - Phase 5: Extended Fuzzing Campaign
- Depends on: #[ISSUE_NUMBER] - Phase 0-3 Completion

---

## Documentation Updates

**Files Added**:
- `ci/generate_toolchain_lock.sh`
- `ci/toolchain.lock`
- `.github/workflows/nightly-canonical.yml`
- `docs/REPRODUCIBLE_BUILDS.md`

**Files Modified**:
- `.github/workflows/ci.yml` (determinism gate added)
- `docs/PHASE_ASSIGNMENTS.md` (Phase 4 status updated)

---

## Next Phase Preview

**Phase 5: Extended Fuzzing Campaign**
- **Duration**: 2-4 weeks (Dec 16 - Jan 10)
- **Owner**: Security Engineer
- **Goal**: 100M+ executions, 0 critical crashes
- **Kickoff**: Week of Dec 16 (parallel with Phase 4 completion)

---

## Attachments

**Required Artifacts** (attach or link):

1. `ci/toolchain.lock` - [Link or attachment]
2. `canonical-manifest.json` - [Link or attachment]
3. `artifacts/checksums.sha256` - [Link or attachment]
4. `logs/determinism.log` - [Link or attachment]
5. `logs/address_san.log` - [Link or attachment]
6. `logs/undefined_san.log` - [Link or attachment]
7. `logs/memory_san.log` - [Link or attachment]
8. GitHub Actions run links:
   - Nightly build 1: [LINK]
   - Nightly build 2: [LINK]
   - Nightly build 3: [LINK]

---

## PR Commands for Reviewers

**Quick verification**:
```bash
# Clone and checkout PR branch
gh pr checkout [PR_NUMBER]

# Verify toolchain lockfile
cat ci/toolchain.lock

# Build twice locally
make clean && make
sha256sum bin/substrate_core bin/glyph_interp > build1.sha256

make clean && make
sha256sum bin/substrate_core bin/glyph_interp > build2.sha256

# Compare
diff build1.sha256 build2.sha256
# Should output nothing (identical)

# Run sanitizers
make sanitizers
# Should show 0 errors

# Trigger nightly workflow
gh workflow run nightly-canonical.yml --ref feature/phase-4-determinism
gh run watch
```

---

**PR Owner**: Build Engineer
**Created**: [DATE]
**Target Merge**: Dec 20, 2025
**Phase**: 4 of 8 (Determinism Hardening)
