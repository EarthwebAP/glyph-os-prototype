#!/bin/sh
#
# Provision GitHub Actions Signing Secrets
# Sets up GPG and Cosign credentials for artifact signing
#

set -e

echo "=== GitHub Actions Secret Provisioning ==="
echo ""

# Check for GitHub CLI
if ! command -v gh > /dev/null 2>&1; then
    echo "ERROR: GitHub CLI (gh) not installed"
    echo "Install: https://cli.github.com/"
    exit 1
fi

# Check authentication
if ! gh auth status > /dev/null 2>&1; then
    echo "ERROR: Not authenticated with GitHub CLI"
    echo "Run: gh auth login"
    exit 1
fi

REPO="EarthwebAP/glyph-os-prototype"

echo "Repository: $REPO"
echo ""

# GPG Key Setup
echo "[1/3] GPG Signing Key Setup"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

if [ -f "$HOME/.gnupg/glyphos-signing-key.asc" ]; then
    echo "Found existing GPG key: $HOME/.gnupg/glyphos-signing-key.asc"
    echo ""
    read -p "Use this key? (y/n): " USE_EXISTING

    if [ "$USE_EXISTING" = "y" ]; then
        GPG_KEY_FILE="$HOME/.gnupg/glyphos-signing-key.asc"
    else
        echo "Please specify GPG key file path:"
        read -p "Path: " GPG_KEY_FILE
    fi
else
    echo "No existing GPG key found. Generating new key..."
    echo ""

    # Generate new GPG key
    cat > /tmp/gpg-keygen-batch << EOF
Key-Type: RSA
Key-Length: 4096
Subkey-Type: RSA
Subkey-Length: 4096
Name-Real: GlyphOS Release
Name-Email: glyphos-release@earthwebap.com
Expire-Date: 2y
Passphrase: $(openssl rand -base64 32)
%commit
EOF

    gpg --batch --gen-key /tmp/gpg-keygen-batch
    rm /tmp/gpg-keygen-batch

    # Export key
    GPG_KEY_ID=$(gpg --list-keys "glyphos-release@earthwebap.com" | grep -A 1 "pub" | tail -1 | tr -d ' ')
    gpg --armor --export-secret-keys "$GPG_KEY_ID" > "$HOME/.gnupg/glyphos-signing-key.asc"
    GPG_KEY_FILE="$HOME/.gnupg/glyphos-signing-key.asc"

    echo "Generated new GPG key: $GPG_KEY_ID"
    echo "Exported to: $GPG_KEY_FILE"
fi

echo ""
echo "Setting GitHub secret: GPG_SIGNING_KEY"
gh secret set GPG_SIGNING_KEY --repo "$REPO" < "$GPG_KEY_FILE"
echo "✓ GPG_SIGNING_KEY provisioned"

# GPG Passphrase
echo ""
read -p "Does this GPG key have a passphrase? (y/n): " HAS_PASSPHRASE

if [ "$HAS_PASSPHRASE" = "y" ]; then
    echo "Enter GPG passphrase (input hidden):"
    stty -echo
    read GPG_PASSPHRASE
    stty echo
    echo ""

    echo "$GPG_PASSPHRASE" | gh secret set GPG_PASSPHRASE --repo "$REPO"
    echo "✓ GPG_PASSPHRASE provisioned"
else
    echo "Skipping GPG_PASSPHRASE (key has no passphrase)"
fi

# Cosign Setup
echo ""
echo "[2/3] Cosign Keyless Signing"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "Cosign keyless signing uses GitHub OIDC tokens (no secret needed)"
echo "Ensure workflow has 'id-token: write' permission"
echo ""
echo "✓ No action required (handled by GitHub Actions OIDC)"

# Optional: KMS/Cloud HSM Setup
echo ""
echo "[3/3] Cloud KMS (Optional)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
read -p "Use cloud KMS for signing? (aws/gcp/azure/none): " KMS_PROVIDER

case "$KMS_PROVIDER" in
    aws)
        echo "AWS KMS Setup:"
        read -p "AWS Access Key ID: " AWS_ACCESS_KEY_ID
        read -p "AWS Secret Access Key (hidden): " -s AWS_SECRET_ACCESS_KEY
        echo ""
        read -p "AWS Region: " AWS_REGION
        read -p "KMS Key ID: " KMS_KEY_ID

        echo "$AWS_ACCESS_KEY_ID" | gh secret set AWS_ACCESS_KEY_ID --repo "$REPO"
        echo "$AWS_SECRET_ACCESS_KEY" | gh secret set AWS_SECRET_ACCESS_KEY --repo "$REPO"
        echo "$AWS_REGION" | gh secret set AWS_REGION --repo "$REPO"
        echo "$KMS_KEY_ID" | gh secret set AWS_KMS_KEY_ID --repo "$REPO"

        echo "✓ AWS KMS credentials provisioned"
        ;;
    gcp)
        echo "GCP KMS Setup:"
        read -p "Service Account JSON path: " GCP_SA_PATH
        read -p "KMS Key Resource ID: " GCP_KMS_KEY

        gh secret set GCP_SA_KEY --repo "$REPO" < "$GCP_SA_PATH"
        echo "$GCP_KMS_KEY" | gh secret set GCP_KMS_KEY --repo "$REPO"

        echo "✓ GCP KMS credentials provisioned"
        ;;
    azure)
        echo "Azure Key Vault Setup:"
        read -p "Azure Tenant ID: " AZURE_TENANT_ID
        read -p "Azure Client ID: " AZURE_CLIENT_ID
        read -p "Azure Client Secret (hidden): " -s AZURE_CLIENT_SECRET
        echo ""
        read -p "Key Vault Name: " AZURE_KEY_VAULT
        read -p "Key Name: " AZURE_KEY_NAME

        echo "$AZURE_TENANT_ID" | gh secret set AZURE_TENANT_ID --repo "$REPO"
        echo "$AZURE_CLIENT_ID" | gh secret set AZURE_CLIENT_ID --repo "$REPO"
        echo "$AZURE_CLIENT_SECRET" | gh secret set AZURE_CLIENT_SECRET --repo "$REPO"
        echo "$AZURE_KEY_VAULT" | gh secret set AZURE_KEY_VAULT --repo "$REPO"
        echo "$AZURE_KEY_NAME" | gh secret set AZURE_KEY_NAME --repo "$REPO"

        echo "✓ Azure Key Vault credentials provisioned"
        ;;
    none)
        echo "Skipping cloud KMS setup"
        ;;
    *)
        echo "Unknown provider: $KMS_PROVIDER"
        ;;
esac

# Summary
echo ""
echo "=== Provisioning Complete ==="
echo ""
echo "Secrets configured:"
echo "  ✓ GPG_SIGNING_KEY"
if [ "$HAS_PASSPHRASE" = "y" ]; then
    echo "  ✓ GPG_PASSPHRASE"
fi
if [ "$KMS_PROVIDER" != "none" ] && [ -n "$KMS_PROVIDER" ]; then
    echo "  ✓ Cloud KMS credentials ($KMS_PROVIDER)"
fi
echo ""

# Verify secrets
echo "Verifying secrets..."
gh secret list --repo "$REPO"

echo ""
echo "Next steps:"
echo "  1. Trigger CI workflow to test signing"
echo "  2. Verify artifacts are signed correctly"
echo "  3. Document secret rotation schedule"
echo ""
echo "Secret Rotation Schedule:"
echo "  - GPG keys: Every 12 months"
echo "  - Cloud KMS: Review quarterly"
echo "  - See docs/OPERATIONALIZATION.md for procedures"
echo ""
