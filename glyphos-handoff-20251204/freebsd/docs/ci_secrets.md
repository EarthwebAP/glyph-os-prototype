# GlyphOS CI Secrets Configuration

This document describes the GitHub Actions secrets required for the GlyphOS CI pipeline.

## Required Secrets

### GPG_PRIVATE_KEY

**Purpose**: Signs release artifacts (checksums, manifests) to ensure authenticity.

**How to generate**:

```bash
# Generate a new GPG key pair
gpg --gen-key
# Follow prompts:
#   - Real name: GlyphOS Release Bot
#   - Email: release@glyphos.example.com
#   - Key type: RSA 4096
#   - Expiration: 2 years

# Export the private key (ASCII armored)
gpg --armor --export-secret-keys release@glyphos.example.com > glyphos_release_private.asc

# ⚠️  IMPORTANT: This file contains your private key. Keep it secure!
```

**How to add to GitHub**:

1. Go to your repository on GitHub
2. Navigate to **Settings** → **Secrets and variables** → **Actions**
3. Click **New repository secret**
4. Name: `GPG_PRIVATE_KEY`
5. Value: Paste the entire contents of `glyphos_release_private.asc`
6. Click **Add secret**

**Format**:
```
-----BEGIN PGP PRIVATE KEY BLOCK-----

lQdGBGUxY...
...
-----END PGP PRIVATE KEY BLOCK-----
```

### GPG_PASSPHRASE (Optional)

**Purpose**: Passphrase for the GPG private key (if key is password-protected).

**How to add**:

1. Go to **Settings** → **Secrets and variables** → **Actions**
2. Click **New repository secret**
3. Name: `GPG_PASSPHRASE`
4. Value: The passphrase you used when creating the GPG key
5. Click **Add secret**

**Note**: If your GPG key has no passphrase, you can skip this secret. However, using a passphrase is recommended for security.

## Optional Secrets

### CODECOV_TOKEN (Future)

For code coverage reporting integration.

### SENTRY_DSN (Future)

For error tracking in production deployments.

## Verifying Secrets are Configured

The CI workflow will:

1. Check if `GPG_PRIVATE_KEY` is set
2. If set, import the key and sign artifacts
3. If not set, skip signing and log a warning

You can verify by checking the CI logs for:

```
✓ GPG key imported successfully
✓ Artifacts signed
```

or

```
⚠️  GPG_PRIVATE_KEY not configured - skipping artifact signing
```

## Key Rotation

GPG keys should be rotated every 2 years:

1. Generate a new key pair
2. Update the `GPG_PRIVATE_KEY` secret
3. Distribute the new public key to users
4. Revoke the old key after a transition period

## Public Key Distribution

The public key should be:

1. Committed to the repository at `ci/keys/glyphos_release.pub.pem`
2. Published on a public keyserver (e.g., keys.openpgp.org)
3. Documented in the README with fingerprint

### Extract Public Key

```bash
# Export public key
gpg --armor --export release@glyphos.example.com > ci/keys/glyphos_release.pub.pem

# Get key fingerprint
gpg --fingerprint release@glyphos.example.com
```

### Publish to Keyserver

```bash
gpg --send-keys <KEY_ID>
```

## Security Best Practices

1. **Never commit private keys** to the repository
2. **Use strong passphrases** for GPG keys
3. **Rotate keys regularly** (every 1-2 years)
4. **Restrict secret access** to repository maintainers only
5. **Audit secret usage** in CI logs regularly
6. **Revoke compromised keys** immediately

## Testing Locally

To test artifact signing locally without configuring secrets:

```bash
# Generate a test key
gpg --batch --gen-key <<EOF
%no-protection
Key-Type: RSA
Key-Length: 2048
Name-Real: GlyphOS Test
Name-Email: test@localhost
Expire-Date: 1y
EOF

# Export for testing
gpg --armor --export-secret-keys test@localhost > /tmp/test_key.asc

# Test signing
export GPG_KEY="$(cat /tmp/test_key.asc)"
echo "$GPG_KEY" | gpg --batch --import
echo "test" > test.txt
gpg --batch --yes --armor --output test.txt.asc --detach-sign test.txt

# Verify
gpg --verify test.txt.asc test.txt
```

## Troubleshooting

### "gpg: signing failed: Inappropriate ioctl for device"

Solution: Add `export GPG_TTY=$(tty)` before signing.

### "gpg: decryption failed: No secret key"

Solution: Verify the private key was imported correctly:

```bash
gpg --list-secret-keys
```

### "gpg: signing failed: No pinentry"

Solution: In CI, use `--batch --yes --pinentry-mode loopback` flags.

## References

- [GitHub Actions Encrypted Secrets](https://docs.github.com/en/actions/security-guides/encrypted-secrets)
- [GPG Manual](https://www.gnupg.org/documentation/manuals/gnupg/)
- [Signing Release Artifacts](https://wiki.debian.org/Creating%20signed%20GitHub%20releases)
