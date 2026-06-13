# Code signing and notarization (macOS Developer ID path)

The Mac App Store is architecturally blocked for CoreMetrics: the App Sandbox denies `proc_listpids`, and no entitlement is available that unlocks it (Apple Developer Forums thread [691857](https://developer.apple.com/forums/thread/691857)). The realistic distribution path is **Developer ID + notarization** — a signed and Apple-notarized binary that users download directly. The user double-clicks the tarball or runs `brew install`, Gatekeeper sees the staple, no scary warning.

This document explains the one-time setup. Once the secrets are in the repo, every `v*` tag automatically produces a signed and notarized macOS binary alongside the existing unsigned one.

## One-time enrollment

1. Enroll in the [Apple Developer Program](https://developer.apple.com/programs/enroll/). $99/yr. You need an Apple ID. The acceptance takes 1–2 business days.

2. In [Apple Developer → Certificates, Identifiers & Profiles](https://developer.apple.com/account/resources/certificates/list), create a **Developer ID Application** certificate. Download the `.cer` and double-click it; it lands in your Keychain.

3. Take note of your **Team ID** from [the membership page](https://developer.apple.com/account#MembershipDetailsCard). Looks like `ABC1234DEF`.

4. Create an [App-Specific Password](https://appleid.apple.com/account/manage) for your Apple ID. Label it `notarytool`. Save it; it is shown once.

## Export the signing identity

From your Mac, with the certificate installed in Keychain:

```bash
# Export both the certificate and its private key. -P sets a temporary
# encryption password the GitHub secret pipeline can use.
security find-identity -v -p codesigning      # confirm the name of the identity
security export -k login.keychain -t identities -f pkcs12 \
                -P "SOME_PASSPHRASE" -o developerid.p12

# Base64 encode for GitHub Secrets (single line)
base64 -i developerid.p12 -o developerid.p12.b64
```

## GitHub Secrets to add

In the repo → Settings → Secrets and variables → Actions, add these four secrets:

| Secret name | Value |
|---|---|
| `MACOS_CODESIGN_IDENTITY_P12` | Contents of `developerid.p12.b64` |
| `MACOS_CODESIGN_IDENTITY_PASSWORD` | `SOME_PASSPHRASE` (the `-P` value above) |
| `MACOS_CODESIGN_IDENTITY_NAME` | The full identity name from `find-identity`, e.g. `"Developer ID Application: Sviatoslav Oleksiienko (ABC1234DEF)"` |
| `MACOS_NOTARIZATION_APPLE_ID` | Your Apple ID email |
| `MACOS_NOTARIZATION_APP_PASSWORD` | The app-specific password from step 4 |
| `MACOS_NOTARIZATION_TEAM_ID` | The Team ID from step 3 |

The `.github/workflows/macos-codesign.yml` workflow reads exactly those names. It is a no-op if any secret is missing, so the workflow can land in the repo before the secrets do without breaking unrelated runs.

## What happens on a release

After the secrets are present, the next `git tag vX.Y.Z && git push origin vX.Y.Z` produces:

1. `coremetrics-vX.Y.Z-macos-arm64.tar.gz` (unsigned, as today)
2. `coremetrics-vX.Y.Z-macos-arm64-signed.tar.gz` (signed + notarized + stapled)

The Homebrew tap formula (`sviatil0/homebrew-coremetrics`) updates to point at the signed tarball; users get a Gatekeeper-clean install with no override clicks. The unsigned tarball stays as a fallback for users who want to verify the binary themselves.

## Verifying a signed binary

```bash
codesign --verify --deep --strict --verbose=2 ./coremetrics
spctl --assess --type execute --verbose ./coremetrics
xcrun stapler validate ./coremetrics-vX.Y.Z-macos-arm64-signed.tar.gz
```

All three should report `accepted` / `valid`.

## Rotating the certificate

Developer ID Application certificates expire after 5 years. When the time comes, regenerate the `.p12` and update `MACOS_CODESIGN_IDENTITY_P12` and `MACOS_CODESIGN_IDENTITY_PASSWORD`. No workflow change required.
