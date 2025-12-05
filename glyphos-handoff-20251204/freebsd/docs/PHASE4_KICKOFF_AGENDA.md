# Phase 4 Kickoff: Determinism Hardening

**Date**: Monday, December 9, 2025
**Time**: 10:00 AM - 11:00 AM PT
**Duration**: 60 minutes
**Location**: Zoom / Conference Room
**Status**: ✅ Ready to Execute

---

## Meeting Objectives

1. ✅ Align team on Phase 4 goals and timeline (1-2 weeks)
2. ✅ Assign specific tasks and owners
3. ✅ Execute first commands live during kickoff
4. ✅ Establish daily standup cadence
5. ✅ Define success criteria and exit gates

---

## Required Attendees

### Core Team (REQUIRED)
- **Build Engineer** - Phase 4 owner, toolchain expert
- **DevOps Lead** - CI/CD and automation
- **Security Engineer** - Code review and validation
- **Project Manager** - Timeline and coordination

### Optional (Recommended)
- **Release Engineer** - Downstream impact assessment
- **QA Lead** - Testing strategy
- **Engineering Manager** - Executive oversight

### Pre-Kickoff Preparation
All attendees should:
- [ ] Review `docs/PHASE_ASSIGNMENTS.md` Phase 4 section
- [ ] Review `docs/IMPLEMENTATION_ROADMAP.md`
- [ ] Have local development environment ready
- [ ] Clone repo: `git clone https://github.com/EarthwebAP/glyph-os-prototype.git`
- [ ] Checkout branch: `git checkout feature/release-readiness`

---

## Agenda (60 minutes)

### 1. Phase 4 Overview (10 min)

**Presenter**: Project Manager

**Topics**:
- ✅ **Goal**: Achieve bit-identical reproducible builds
- ✅ **Why**: Supply chain security, third-party verification, determinism gates
- ✅ **Duration**: 1-2 weeks (Dec 9 - Dec 20)
- ✅ **Success**: 3 consecutive nightly builds match canonical manifest

**Acceptance Criteria**:
```
✅ Three consecutive nightly builds produce identical SHA256 checksums
✅ Toolchain lockfile (ci/toolchain.lock) committed and enforced
✅ All reproducibility env vars (SOURCE_DATE_EPOCH, TZ, LANG) in CI
✅ Nightly parity check job running and passing
✅ CI gate blocks non-deterministic builds from merging
```

**Risks**:
- ⚠️ Toolchain changes break determinism → Pin all versions
- ⚠️ New dependencies introduce non-determinism → Review all changes
- ⚠️ Timestamps leak into binaries → SOURCE_DATE_EPOCH enforcement

---

### 2. Task Assignments (10 min)

**Presenter**: Build Engineer

| Task | Owner | Deadline | Dependencies |
|------|-------|----------|--------------|
| 4.1: Create toolchain lockfile | Build Engineer | Dec 10 (Tue) | None |
| 4.2: Add SOURCE_DATE_EPOCH to CI | DevOps Lead | Dec 11 (Wed) | Task 4.1 |
| 4.3: Set up nightly canonical builds | DevOps Lead | Dec 12 (Thu) | Task 4.2 |
| 4.4: Implement build comparison script | Build Engineer | Dec 13 (Fri) | Task 4.3 |
| 4.5: Add CI determinism gate | DevOps Lead | Dec 16 (Mon) | Task 4.4 |
| 4.6: First nightly run validation | Build Engineer | Dec 17 (Tue) | Task 4.5 |
| 4.7: Second nightly run | Build Engineer | Dec 18 (Wed) | Task 4.6 |
| 4.8: Third nightly run (sign-off) | Build Engineer | Dec 19 (Thu) | Task 4.7 |
| 4.9: Documentation and handoff | Build Engineer | Dec 20 (Fri) | Task 4.8 |

**Critical Path**: Tasks 4.1 → 4.2 → 4.3 → 4.4 → 4.5 (must complete by Dec 16)

---

### 3. Live Command Execution (20 min)

**Presenter**: Build Engineer (screen share)

#### 3.1: Generate Toolchain Lockfile (5 min)

**Command**:
```bash
cd /path/to/glyphos-handoff-20251204/freebsd

# Create toolchain lock script
cat > ci/generate_toolchain_lock.sh << 'EOF'
#!/bin/sh
#
# Generate reproducible toolchain lockfile
#

set -e

echo "# GlyphOS Toolchain Lockfile"
echo "# Generated: $(date -u +%Y-%m-%d_%H:%M:%S_UTC)"
echo "# Format: package_name==version"
echo ""

# FreeBSD packages
echo "## FreeBSD Packages"
pkg info clang llvm build-essential jq openssl | grep -E '^(clang|llvm|gcc|binutils|jq|openssl)' | \
    awk '{print $1}' | sort

echo ""
echo "## Compiler Versions"
echo "gcc_version=$(gcc --version | head -1 | awk '{print $NF}')"
echo "clang_version=$(clang --version | head -1 | awk '{print $NF}')"
echo "ld_version=$(ld --version | head -1 | awk '{print $NF}')"

echo ""
echo "## Checksums"
echo "gcc_sha256=$(sha256sum $(which gcc) | awk '{print $1}')"
echo "clang_sha256=$(sha256sum $(which clang) | awk '{print $1}')"

echo ""
echo "## Environment"
echo "SOURCE_DATE_EPOCH=1701820800"
echo "TZ=UTC"
echo "LANG=C"
echo "LC_ALL=C"
EOF

chmod +x ci/generate_toolchain_lock.sh

# Generate lockfile
./ci/generate_toolchain_lock.sh > ci/toolchain.lock

# Review
cat ci/toolchain.lock
```

**Expected Output**:
```
# GlyphOS Toolchain Lockfile
# Generated: 2025-12-09_18:00:00_UTC

## FreeBSD Packages
clang-16.0.6
gcc-12.2.0
llvm-16.0.6
...

## Compiler Versions
gcc_version=12.2.0
clang_version=16.0.6
...
```

**Action**: Commit lockfile immediately
```bash
git add ci/generate_toolchain_lock.sh ci/toolchain.lock
git commit -m "ci: add toolchain lockfile for reproducible builds"
git push origin feature/release-readiness
```

---

#### 3.2: Update CI with Reproducibility Env Vars (5 min)

**Command**:
```bash
# Verify current .github/workflows/ci.yml has these vars
grep -A 5 "^env:" .github/workflows/ci.yml

# Should show:
# env:
#   TZ: UTC
#   LANG: C
#   LC_ALL: C
#   SOURCE_DATE_EPOCH: 1701820800
#   GDF_SEED: 0

# If missing, add them
```

**Already present** ✅ in current workflow from Phase 2 work.

---

#### 3.3: Create Nightly Canonical Build Job (10 min)

**Command**:
```bash
cat > .github/workflows/nightly-canonical.yml << 'EOF'
name: Nightly Canonical Build

on:
  schedule:
    - cron: '0 2 * * *'  # 2 AM UTC daily
  workflow_dispatch:  # Manual trigger for testing

env:
  TZ: UTC
  LANG: C
  LC_ALL: C
  SOURCE_DATE_EPOCH: 1701820800
  GDF_SEED: 0

jobs:
  canonical-build:
    name: Canonical Deterministic Build
    runs-on: ubuntu-latest

    steps:
      - name: Checkout code
        uses: actions/checkout@v4

      - name: Verify toolchain lockfile
        run: |
          cd freebsd
          if [ ! -f ci/toolchain.lock ]; then
            echo "ERROR: ci/toolchain.lock not found"
            exit 1
          fi
          echo "Toolchain lockfile present:"
          cat ci/toolchain.lock

      - name: Install exact toolchain versions
        run: |
          cd freebsd
          # Parse toolchain.lock and install exact versions
          # (In production, use apt-get install <package>=<version>)
          sudo apt-get update
          sudo apt-get install -y clang llvm build-essential

      - name: Canonical build
        run: |
          cd freebsd
          mkdir -p bin

          # Build with deterministic flags
          clang -O2 -o bin/substrate_core src/substrate_core.c -lm
          clang -O2 -o bin/glyph_interp src/glyph_interpreter.c -lm

          strip bin/substrate_core bin/glyph_interp

          # Generate checksums
          sha256sum bin/substrate_core bin/glyph_interp | tee canonical-checksums.sha256

      - name: Save canonical manifest
        run: |
          cd freebsd

          cat > canonical-manifest.json << EOF
          {
            "build_date": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
            "git_commit": "${{ github.sha }}",
            "git_branch": "${{ github.ref_name }}",
            "substrate_core_sha256": "$(sha256sum bin/substrate_core | awk '{print $1}')",
            "glyph_interp_sha256": "$(sha256sum bin/glyph_interp | awk '{print $1}')",
            "toolchain_lock_sha256": "$(sha256sum ci/toolchain.lock | awk '{print $1}')"
          }
          EOF

          cat canonical-manifest.json

      - name: Upload canonical artifacts
        uses: actions/upload-artifact@v4
        with:
          name: canonical-build-${{ github.run_number }}
          path: |
            freebsd/bin/substrate_core
            freebsd/bin/glyph_interp
            freebsd/canonical-checksums.sha256
            freebsd/canonical-manifest.json
          retention-days: 90

      - name: Compare with previous canonical build
        if: github.event_name == 'schedule'
        run: |
          cd freebsd

          # Download previous canonical build artifact (requires gh CLI)
          # For first run, this will fail gracefully
          if gh run list --workflow=nightly-canonical.yml --status=success --limit=2 --json databaseId --jq '.[1].databaseId' > /dev/null 2>&1; then
            PREV_RUN=$(gh run list --workflow=nightly-canonical.yml --status=success --limit=2 --json databaseId --jq '.[1].databaseId')

            echo "Comparing with previous run: $PREV_RUN"
            gh run download "$PREV_RUN" --name "canonical-build-*" --dir previous/

            # Compare checksums
            if diff canonical-checksums.sha256 previous/canonical-checksums.sha256; then
              echo "✅ DETERMINISM VERIFIED: Builds are identical"
            else
              echo "❌ DETERMINISM FAILED: Builds differ"
              echo "Current:"
              cat canonical-checksums.sha256
              echo "Previous:"
              cat previous/canonical-checksums.sha256
              exit 1
            fi
          else
            echo "First canonical build - no previous build to compare"
          fi
EOF

# Commit nightly workflow
git add .github/workflows/nightly-canonical.yml
git commit -m "ci: add nightly canonical build for determinism verification"
git push origin feature/release-readiness
```

**Test**: Trigger manually during kickoff
```bash
gh workflow run nightly-canonical.yml --ref feature/release-readiness
gh run watch
```

---

### 4. Daily Standup Cadence (5 min)

**Presenter**: Project Manager

**Schedule**:
- **Time**: 9:00 AM PT daily
- **Duration**: 15 minutes
- **Format**: Slack huddle in #glyphos-phases-4-8
- **Attendees**: Build Engineer, DevOps Lead, PM

**Standup Template**:
```
🔨 Phase 4 Daily Standup - Dec [DATE]

Build Engineer:
✅ Yesterday: [completed tasks]
🚀 Today: [planned tasks]
🚫 Blockers: [any blockers]

DevOps Lead:
✅ Yesterday: [completed tasks]
🚀 Today: [planned tasks]
🚫 Blockers: [any blockers]

Status:
- Days remaining: [X/10]
- Tasks complete: [X/9]
- On track: YES/NO
```

---

### 5. Success Criteria Review (5 min)

**Presenter**: Build Engineer

**Exit Criteria for Phase 4**:

```bash
# Check 1: Toolchain lockfile exists and committed
test -f ci/toolchain.lock && echo "✅ Toolchain lockfile present"

# Check 2: Reproducibility vars in CI
grep "SOURCE_DATE_EPOCH" .github/workflows/ci.yml && echo "✅ Reproducibility vars present"

# Check 3: Nightly workflow exists
test -f .github/workflows/nightly-canonical.yml && echo "✅ Nightly workflow present"

# Check 4: Three consecutive identical builds
# Manual verification required - check GitHub Actions runs
echo "⏳ Awaiting three consecutive nightly builds"
```

**Metrics to Track**:
- Build determinism rate: Target 100% (3/3 identical)
- Build time variance: Target < 5%
- False positive rate: Target < 1% (determinism gate blocking valid builds)

**Sign-off Required**:
- [ ] Build Engineer: "Phase 4 acceptance criteria met"
- [ ] DevOps Lead: "CI determinism gate operational"
- [ ] Security Engineer: "Toolchain lockfile reviewed and approved"
- [ ] Project Manager: "Timeline and deliverables complete"

---

### 6. Q&A and Next Steps (10 min)

**Open Discussion**

**Questions to Address**:
- What happens if a nightly build fails determinism?
  - **Answer**: Investigate immediately, create bug ticket, freeze merges until fixed

- How do we handle emergency hotfixes during Phase 4?
  - **Answer**: Toolchain lockfile allows rebuilding exact environment

- What if we need to update the toolchain?
  - **Answer**: Update lockfile, re-run 3 nightly builds for verification

**Next Steps After Kickoff**:
1. Build Engineer: Execute Task 4.1 today (toolchain lockfile)
2. DevOps Lead: Schedule pairing session for Task 4.2-4.3
3. All: Monitor #glyphos-phases-4-8 for updates
4. PM: Send meeting notes and action items within 2 hours

---

## Post-Kickoff Action Items

**Immediate (Today - Dec 9)**:
- [ ] Build Engineer: Generate and commit toolchain lockfile
- [ ] DevOps Lead: Test nightly workflow with manual trigger
- [ ] Security Engineer: Review toolchain lockfile for suspicious packages
- [ ] PM: Send meeting notes to stakeholders

**This Week (Dec 9-13)**:
- [ ] Complete Tasks 4.1 through 4.4
- [ ] First nightly canonical build runs successfully
- [ ] Determinism comparison script validated

**Next Week (Dec 16-20)**:
- [ ] CI determinism gate implemented
- [ ] Three consecutive nightly builds pass
- [ ] Phase 4 completion sign-off
- [ ] Handoff to Phase 5 (Fuzzing)

---

## Communication Channels

**Slack**:
- `#glyphos-phases-4-8` - General updates and discussions
- `#glyphos-determinism` - Phase 4 specific channel (create if doesn't exist)

**GitHub**:
- Issues: Label with `phase-4-determinism`
- PRs: Prefix with `[Phase 4]`
- Discussions: Use GitHub Discussions for design questions

**Email**:
- Weekly status to stakeholders (Fridays)
- Immediate escalation for blockers

---

## References

**Documentation**:
- `docs/PHASE_ASSIGNMENTS.md` - Complete phase breakdown
- `docs/IMPLEMENTATION_ROADMAP.md` - Overall timeline
- `docs/IMMEDIATE_ACTIONS_COMPLETE.md` - Pre-Phase 4 status

**Related Work**:
- Reproducible Builds: https://reproducible-builds.org/
- SLSA Framework: https://slsa.dev/
- Debian Reproducible: https://wiki.debian.org/ReproducibleBuilds

---

## Emergency Contacts

**Phase 4 Team**:
- Build Engineer: [NAME] - [EMAIL] - [PHONE]
- DevOps Lead: [NAME] - [EMAIL] - [PHONE]
- Security Engineer: [NAME] - [EMAIL] - [PHONE]
- Project Manager: [NAME] - [EMAIL] - [PHONE]

**Escalation**:
- P0 Blocker: Page on-call via PagerDuty
- P1 Issue: Slack @channel in #glyphos-phases-4-8
- P2 Issue: Create GitHub issue with `phase-4-determinism` label

---

**Meeting Owner**: Project Manager
**Last Updated**: 2025-12-05
**Next Review**: Post-kickoff retrospective (Dec 20, 2025)
