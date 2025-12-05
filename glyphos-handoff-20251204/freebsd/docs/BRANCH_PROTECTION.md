# GitHub Branch Protection Configuration

**Repository**: EarthwebAP/glyph-os-prototype
**Status**: 📋 Configuration Guide
**Last Updated**: 2025-12-05

---

## Overview

This document provides GitHub branch protection rules to ensure code quality, security, and reliability before merging to protected branches.

---

## Protected Branches

### main

**Purpose**: Production-ready code only

**Required Checks**:
- [x] build-and-test (gcc)
- [x] build-and-test (clang)
- [x] sanitizers (address)
- [x] sanitizers (undefined)
- [x] sanitizers (memory)
- [x] determinism-check
- [x] security-scan

**Additional Requirements**:
- [x] Require pull request before merging
- [x] Require 2 approvals
- [x] Dismiss stale reviews
- [x] Require review from code owners
- [x] Require status checks to pass
- [x] Require branches to be up to date
- [x] Require signed commits
- [x] Include administrators
- [x] Restrict force pushes
- [x] Restrict deletions

### develop

**Purpose**: Integration branch for features

**Required Checks**:
- [x] build-and-test (gcc)
- [x] build-and-test (clang)
- [x] security-scan

**Additional Requirements**:
- [x] Require pull request before merging
- [x] Require 1 approval
- [x] Require status checks to pass
- [x] Restrict force pushes

### feature/*

**Purpose**: Feature development branches

**Required Checks**:
- [x] build-and-test (gcc OR clang)

**Additional Requirements**:
- [x] Require status checks to pass
- [ ] No force push restrictions (allowed for development)

---

## GitHub Configuration Steps

### 1. Navigate to Repository Settings

```
GitHub.com → Repository → Settings → Branches → Add branch protection rule
```

### 2. Configure 'main' Branch

**Branch name pattern**: `main`

**Protection Rules**:

```yaml
# Pull Request Requirements
☑ Require a pull request before merging
  ☑ Require approvals: 2
  ☑ Dismiss stale pull request approvals when new commits are pushed
  ☑ Require review from Code Owners
  ☑ Require approval of the most recent reviewable push

# Status Checks
☑ Require status checks to pass before merging
  ☑ Require branches to be up to date before merging

  Required status checks:
  - build-and-test (gcc)
  - build-and-test (clang)
  - sanitizers (address)
  - sanitizers (undefined)
  - sanitizers (memory)
  - determinism-check
  - security-scan
  - summary

# Commit Signing
☑ Require signed commits

# Other Rules
☑ Require linear history
☑ Include administrators
☑ Restrict who can push to matching branches (Administrators only)
☑ Allow force pushes: NEVER
☑ Allow deletions: NEVER
```

### 3. Configure 'develop' Branch

**Branch name pattern**: `develop`

```yaml
☑ Require a pull request before merging
  ☑ Require approvals: 1

☑ Require status checks to pass before merging
  Required checks:
  - build-and-test (gcc)
  - build-and-test (clang)
  - security-scan

☑ Restrict who can push to matching branches (Maintainers only)
☑ Allow force pushes: NEVER
```

### 4. Configure 'feature/*' Branches

**Branch name pattern**: `feature/*`

```yaml
☑ Require status checks to pass before merging
  Required checks:
  - build-and-test (gcc)

# More permissive for development
☐ Allow force pushes: Allowed (for rebasing)
```

---

## CODEOWNERS Configuration

Create `.github/CODEOWNERS`:

```
# GlyphOS Code Owners

# Default owners for everything
*                           @EarthwebAP

# Core substrate implementation
/freebsd/src/substrate_core.c  @EarthwebAP @security-team

# Glyph interpreter
/freebsd/src/glyph_interpreter.c  @EarthwebAP @app-team

# Security-critical code
/freebsd/src/security_utils.*  @EarthwebAP @security-team

# CI/CD and workflows
/.github/workflows/*         @EarthwebAP @devops-team

# Monitoring and observability
/freebsd/monitoring/*        @EarthwebAP @sre-team
/freebsd/src/metrics.*       @EarthwebAP @sre-team

# Documentation
/freebsd/docs/*              @EarthwebAP @doc-team

# Runbooks (require SRE review)
/freebsd/docs/runbooks/*     @EarthwebAP @sre-team
```

---

## Required Checks Configuration

### Status Check Sources

All checks come from `.github/workflows/ci.yml`:

```yaml
jobs:
  build-and-test:  # Matrix: gcc, clang
  sanitizers:      # Matrix: address, undefined, memory
  determinism-check:
  security-scan:
  sign-artifacts:
  summary:
```

### Check Timeout Settings

```yaml
# In GitHub Settings → Actions → General
Default workflow timeout: 60 minutes
Default job timeout: 30 minutes
```

---

## Bypass Procedures

### Emergency Hotfix

**When to use**: P0 production incident requiring immediate fix

**Procedure**:
1. Create branch: `hotfix/description`
2. Make minimal fix
3. Request emergency review from 2+ senior engineers
4. Merge with administrator override
5. Post-merge:
   - Full CI run required
   - Incident report within 24 hours
   - Retrospective within 1 week

**Documentation**: Document override in `docs/HOTFIX_LOG.md`

### Dependency Updates

**Automated**: Dependabot PRs can auto-merge if:
- All CI checks pass
- Security scan clean
- Patch version updates only (x.y.Z)

**Manual**: Minor/major updates require human review

---

## Enforcement

### Pre-commit Hooks

Install locally to catch issues before push:

```bash
# Install pre-commit
pip install pre-commit

# Install hooks
cd /path/to/repo
pre-commit install

# Hooks run on git commit
```

**`.pre-commit-config.yaml`**:
```yaml
repos:
  - repo: https://github.com/pre-commit/pre-commit-hooks
    rev: v4.4.0
    hooks:
      - id: trailing-whitespace
      - id: end-of-file-fixer
      - id: check-added-large-files
        args: ['--maxkb=1000']
      - id: check-merge-conflict

  - repo: local
    hooks:
      - id: build-test
        name: Build and test
        entry: make test
        language: system
        pass_filenames: false
```

---

## Monitoring and Alerts

### GitHub Actions

**Alert on**:
- CI failures on main/develop
- Security scan findings
- Missed required checks
- Branch protection violations

**Notification channels**:
- Slack: #github-ci
- Email: ci-alerts@glyphos.local

### Metrics to Track

```
github_ci_success_rate
github_ci_duration_seconds
github_pr_review_time_hours
github_branch_protection_violations
```

---

## Troubleshooting

### "Required status check is not running"

**Cause**: Check name mismatch

**Fix**:
```bash
# List actual check names from recent runs
gh pr checks <PR_NUMBER>

# Update branch protection with exact names
```

### "Cannot force push to protected branch"

**Expected**: Working as designed

**Workaround**:
```bash
# Create new branch from updated main
git checkout -b feature/new-branch origin/main
git cherry-pick <commits>
```

### "Administrator override required"

**When needed**:
- Emergency hotfixes
- CI infrastructure issues
- False positive security scans

**Log**: Document all overrides in `docs/OVERRIDE_LOG.md`

---

## Rollout Plan

### Phase 1: Soft Enforcement (Week 1)

- Configure branch protection
- Make checks required but allow bypasses
- Monitor for false positives
- Tune thresholds

### Phase 2: Hard Enforcement (Week 2)

- Remove bypass permissions
- Require all checks
- Include administrators
- Full enforcement

---

## Success Metrics

**Target**:
- 100% CI pass rate on main
- 0 hotfix overrides per month (after stabilization)
- <2 hours mean PR review time
- 95% signed commit rate

---

## References

- GitHub Branch Protection: https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-protected-branches
- CODEOWNERS: https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/about-code-owners
- CI Workflow: `.github/workflows/ci.yml`
