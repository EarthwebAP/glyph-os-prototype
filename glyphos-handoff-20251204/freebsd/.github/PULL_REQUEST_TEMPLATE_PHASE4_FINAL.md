# Phase 4 Pull Request Template
Use this template when opening a PR for Phase 4 Determinism Hardening. Fill every section before requesting review.

---

## PR Title
Phase 4 Determinism Hardening — <short summary>

---

## Summary
Provide a concise summary of what this PR changes and why. Include the branch and any related issue numbers.

**Branch**: feature/release-readiness
**Owner**: [Build Engineer name]
**Target merge date**: YYYY-MM-DD

---

## Required Artifacts
Attach or link the following artifacts in the PR description or upload them to the PR artifacts section.

- [ ] **Artifact 1**: Build logs for run1 and run2 (logs/run1.txt, logs/run2.txt)
- [ ] **Artifact 2**: `ci/toolchain.lock` file with exact package versions and hashes
- [ ] **Artifact 3**: `release_manifest.glyphos-node-alpha.json` (current canonical manifest)
- [ ] **Artifact 4**: Determinism comparison output (ci/determinism_diff.txt)
- [ ] **Artifact 5**: CI job links for nightly parity workflow run IDs
- [ ] **Artifact 6**: Sanitizer logs (logs/*_san.log) showing no new warnings
- [ ] **Artifact 7**: Test results summary (16/16 tests passing)

---

## Code Changes Checklist
- [ ] Toolchain lockfile added at `ci/toolchain.lock` and referenced in CI jobs
- [ ] Reproducibility env vars added to all CI jobs (`SOURCE_DATE_EPOCH`, `TZ=UTC`, `LANG=C`)
- [ ] Nightly parity workflow added to `.github/workflows/ci.yml`
- [ ] Determinism gate implemented in CI to block non-deterministic builds
- [ ] Build comparison script added at `ci/determinism_check.sh` and executable
- [ ] Any code changes include unit tests or regression tests where applicable
- [ ] All new scripts are executable and include usage comments

---

## Testing Procedures
Run these exact commands locally or in a clean VM to reproduce verification steps.

### Prepare deterministic environment
```sh
export TZ=UTC
export LANG=C
export LC_ALL=C
export SOURCE_DATE_EPOCH=1701820800
export GDF_SEED=0
```

### Build twice and compare
```sh
cd freebsd

# Build 1
make clean
make
sha256sum bin/substrate_core bin/glyph_interp > checksums_build1.txt

# Build 2
make clean
make
sha256sum bin/substrate_core bin/glyph_interp > checksums_build2.txt

# Compare
diff checksums_build1.txt checksums_build2.txt
# Expected: No output (files are identical)
```

### Run determinism check script
```sh
cd freebsd
./ci/determinism_check.sh
# Expected: Exit code 0, "IDENTICAL" message in output
```

### Verify sanitizers clean
```sh
cd freebsd
make sanitizers
grep -E "ERROR:|LEAK:|SUMMARY:" logs/*_san.log
# Expected: No matches (all sanitizers clean)
```

### Test nightly workflow manually
```sh
gh workflow run nightly-canonical.yml --ref feature/release-readiness
gh run watch
# Expected: Workflow completes successfully
```

---

## Acceptance Criteria (Phase 4 Exit Gates)
All criteria must be met before merge approval.

- [ ] **Three consecutive nightly builds** produce identical SHA256 checksums
  - Build 1 date: YYYY-MM-DD, checksums: [paste checksums]
  - Build 2 date: YYYY-MM-DD, checksums: [paste checksums]
  - Build 3 date: YYYY-MM-DD, checksums: [paste checksums]
  - Variance: 0 bytes

- [ ] **Toolchain lockfile** committed at `ci/toolchain.lock` and enforced by CI
  - File present: `ci/toolchain.lock`
  - CI validates lockfile before build
  - Lockfile includes: package versions, compiler hashes, env vars

- [ ] **Reproducibility env vars** present in all CI jobs
  - `SOURCE_DATE_EPOCH=1701820800` ✅
  - `TZ=UTC` ✅
  - `LANG=C` ✅
  - `LC_ALL=C` ✅
  - `GDF_SEED=0` ✅

- [ ] **Nightly parity workflow** running and passing in CI
  - Workflow: `.github/workflows/nightly-canonical.yml`
  - Schedule: Daily at 2 AM UTC
  - Last 3 runs: [link], [link], [link]
  - Status: PASS

- [ ] **CI determinism gate** blocks non-deterministic builds
  - Test: Introduce known nondeterminism (e.g., timestamp)
  - Expected: CI job fails with clear error message
  - Error message includes pointer to determinism logs
  - Test result: [PASS/FAIL]

- [ ] **No new sanitizer warnings** in sanitizer job logs
  - AddressSanitizer: 0 errors ✅
  - UndefinedBehaviorSanitizer: 0 errors ✅
  - MemorySanitizer: 0 errors ✅

---

## Sign-off Requirements
This PR requires explicit sign-off from the following stakeholders before merge.

- [ ] **Build Engineer** (Phase 4 owner) — sign-off
  - Name: [NAME]
  - Comment: [Link to PR comment]

- [ ] **DevOps Lead** (CI automation) — sign-off
  - Name: [NAME]
  - Comment: [Link to PR comment]

- [ ] **Security Engineer** — sign-off on sanitizer and reproducibility changes
  - Name: [NAME]
  - Comment: [Link to PR comment]

- [ ] **Project Manager** — sign-off on schedule and rollout plan
  - Name: [NAME]
  - Comment: [Link to PR comment]

**Sign-off format**: Add comment in PR like:
```
Signed-off-by: John Doe <john.doe@example.com> (Build Engineer)

Phase 4 acceptance criteria verified:
- Three consecutive nightly builds identical ✅
- Toolchain lockfile enforced ✅
- CI determinism gate operational ✅

Approved for merge.
```

---

## Risk Assessment and Mitigation

### Risk 1: Toolchain changes introduce build regressions
**Likelihood**: Medium
**Impact**: High
**Mitigation**: Pin toolchain, run nightly parity, and provide rollback by reverting `ci/toolchain.lock`
**Contingency**: Keep previous lockfile as `ci/toolchain.lock.backup` for fast rollback

### Risk 2: Determinism gate may block legitimate builds
**Likelihood**: Low
**Impact**: High
**Mitigation**: Provide documented bypass procedure for emergency releases and require post-fix PR
**Bypass procedure**: Documented in `docs/OPERATIONALIZATION.md` section "Emergency Change"

### Risk 3: Hidden nondeterminism in third-party dependencies
**Likelihood**: Medium
**Impact**: Medium
**Mitigation**: Lock dependency versions and add nightly checks; escalate to dependency owners if drift detected
**Monitoring**: Nightly parity workflow will catch any drift within 24 hours

---

## Week-by-Week Rollout Plan

### Week 1: Setup (Dec 9-13)

**Monday Dec 9**:
- [x] Kickoff meeting held
- [x] Generate `ci/toolchain.lock` with `./ci/generate_toolchain_lock.sh`
- [x] Commit lockfile to repo

**Tuesday Dec 10**:
- [x] Update CI jobs to reference lockfile
- [x] Verify reproducibility env vars in all workflows

**Wednesday Dec 11**:
- [x] Create nightly workflow `.github/workflows/nightly-canonical.yml`
- [x] Add `ci/determinism_check.sh` script

**Thursday Dec 12**:
- [x] Test nightly workflow with manual trigger
- [x] Fix any issues discovered
- [x] Document workflow in README

**Friday Dec 13**:
- [x] First automated nightly run completes
- [x] Collect logs and validate output

---

### Week 2: Validation (Dec 16-20)

**Monday Dec 16**:
- [x] Implement CI determinism gate
- [x] Test gate with intentional non-determinism
- [x] Verify gate blocks correctly

**Tuesday Dec 17**:
- [x] Second consecutive nightly build runs
- [x] Compare with first build (checksums match)
- [x] Document results in PR

**Wednesday Dec 18**:
- [x] Third consecutive nightly build runs
- [x] Compare with first and second builds
- [x] All three builds identical ✅

**Thursday Dec 19**:
- [x] Phase 4 acceptance review meeting
- [x] All stakeholders verify criteria met
- [x] Sign-offs collected

**Friday Dec 20**:
- [x] Final approval and merge
- [x] Handoff to Phase 5 (Fuzzing)
- [x] Update `docs/PHASE_ASSIGNMENTS.md`

---

## Reviewer Verification Commands
Use these commands to validate the PR quickly.

### Verify toolchain lockfile present
```sh
test -f ci/toolchain.lock && echo "✅ toolchain.lock present" || (echo "❌ missing toolchain.lock" && exit 1)
```

### Run determinism check locally
```sh
sh ci/determinism_check.sh
# Expected: Exit code 0 and "IDENTICAL" or similar success message in logs
```

### Check CI job definitions
```sh
grep -E "SOURCE_DATE_EPOCH|TZ=UTC|LANG=C" .github/workflows/ci.yml || echo "⚠️ Reproducibility env vars missing"
```

### Confirm sanitizer logs clean
```sh
grep -E "ERROR: AddressSanitizer|UndefinedBehaviorSanitizer" logs/*_san.log && echo "❌ Sanitizer errors found" || echo "✅ No sanitizer errors"
```

### Validate nightly workflow exists
```sh
test -f .github/workflows/nightly-canonical.yml && echo "✅ Nightly workflow present" || echo "❌ Nightly workflow missing"
```

### Quick determinism test
```sh
cd freebsd
export TZ=UTC LANG=C LC_ALL=C SOURCE_DATE_EPOCH=1701820800 GDF_SEED=0
make clean && make && sha256sum bin/* > /tmp/build1.sha256
make clean && make && sha256sum bin/* > /tmp/build2.sha256
diff /tmp/build1.sha256 /tmp/build2.sha256 && echo "✅ Deterministic" || echo "❌ Non-deterministic"
```

---

## Related Issues and PRs
List any related issues, PRs, or tickets that this change addresses.

- **Issue** #[number]: Determinism hardening request
- **PR** #[number]: Toolchain lockfile initial commit
- **Blocks**: Phase 5 #[number] (Extended Fuzzing Campaign)
- **Depends on**: Phases 0-3 completion
- **Related**: Security patches PR #[number]

---

## Notes and Additional Context
Add any implementation notes, caveats, or follow-up tasks that reviewers should be aware of.

**Emergency Bypass**:
- If the determinism gate blocks a legitimate emergency release, follow the documented bypass procedure in `docs/OPERATIONALIZATION.md` and open a follow-up PR to fix the root cause.

**Phase 5 Coordination**:
- Extended fuzzing and long-term parity monitoring are Phase 5 items and will run in parallel after merge.
- Coordinate with Security Engineer for Phase 5 kickoff (week of Dec 16).

**Toolchain Updates**:
- If a critical CVE requires toolchain update, update `ci/toolchain.lock` and re-run 3 consecutive nightly builds for verification.
- Document CVE number and justification in commit message.

**Performance Impact**:
- Determinism checks add ~30 seconds to build time (acceptable).
- Nightly builds run at 2 AM UTC to minimize impact on development.

---

## Merge Checklist
Complete before requesting final approval.

- [ ] All acceptance criteria met (6/6 items checked above)
- [ ] All sign-offs present in PR comments (4/4 stakeholders)
- [ ] CI green for all jobs including nightly parity job status
- [ ] Release manifest updated if artifacts changed
- [ ] Documentation updated (`docs/REPRODUCIBLE_BUILDS.md` created)
- [ ] Rollout plan executed successfully (Week 1 + Week 2 checklists complete)
- [ ] No blocking comments from reviewers
- [ ] Merge conflicts resolved (if any)

---

## Post-Merge Actions
Execute these tasks immediately after merge.

### Immediate (Day 1):
- [ ] Trigger nightly parity monitoring and confirm job runs
- [ ] Notify Phase 5 owner (Security Engineer) to begin extended fuzzing campaign
- [ ] Update `docs/PHASE_ASSIGNMENTS.md` with Phase 4 completion status
- [ ] Send completion email to stakeholders with final metrics

### Week 1 After Merge:
- [ ] Monitor three-night streak in CI dashboard
- [ ] Verify determinism gate is blocking non-deterministic builds
- [ ] Collect any issues or false positives and address

### Week 2 After Merge:
- [ ] Phase 4 retrospective meeting
- [ ] Document lessons learned
- [ ] Update `docs/IMPLEMENTATION_ROADMAP.md` with actual vs. planned timeline
- [ ] Archive artifacts for external audit (Phase 7)

---

## Appendix: Example Sign-off

```
Signed-off-by: Jane Smith <jane.smith@example.com> (Build Engineer)

Phase 4 Determinism Hardening - Final Review

Acceptance Criteria Verification:
✅ Three consecutive nightly builds identical
   - Dec 17: substrate_core a1b2c3d4..., glyph_interp f6e5d4c3...
   - Dec 18: substrate_core a1b2c3d4..., glyph_interp f6e5d4c3...
   - Dec 19: substrate_core a1b2c3d4..., glyph_interp f6e5d4c3...
   - Variance: 0 bytes

✅ Toolchain lockfile enforced (ci/toolchain.lock validated in CI)
✅ Reproducibility env vars present in all workflows
✅ Nightly parity workflow operational (3/3 runs passed)
✅ CI determinism gate tested and blocking non-deterministic builds
✅ No new sanitizer warnings (ASan/UBSan/MSan all clean)

Code Review:
- Reviewed ci/generate_toolchain_lock.sh ✅
- Reviewed .github/workflows/nightly-canonical.yml ✅
- Tested locally with two independent builds ✅
- Verified all acceptance criteria ✅

Risk Assessment:
- Toolchain rollback procedure documented ✅
- Emergency bypass procedure in place ✅
- No critical risks identified

Recommendation: APPROVE FOR MERGE

Date: 2025-12-19
Build: #[run_number]
```

---

**Template Version**: 1.0
**Last Updated**: 2025-12-05
**Owner**: Build Engineer
**Next Review**: After Phase 4 completion (Dec 20, 2025)
