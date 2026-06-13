# Packaging

Distribution channels and their current status. Channels marked **autonomous** can be cut and pushed by the existing release workflow without per-release human action; channels marked **gated** need an account, credential, or external review the maintainer has to complete once.

## Active (autonomous)

| Channel | Where | Notes |
|---|---|---|
| GitHub Releases tarball | `coremetrics-vX.Y.Z-<platform>.tar.gz` | Built by `.github/workflows/release.yml` on every `v*` tag. macOS arm64 + Linux x86_64 today. |
| GitHub Releases `.deb` | `coremetrics_X.Y.Z_amd64.deb` | Built by the same workflow on the Linux runner. Declares `libsdl3-*` runtime depends. |
| Homebrew tap | [`sviatil0/homebrew-coremetrics`](https://github.com/sviatil0/homebrew-coremetrics) | Formula reads the tarball URL + SHA256 from the latest release. Update via `Formula/coremetrics.rb` after each tag. |

## Staged (autonomous build, gated publish)

| Channel | Where | Blocker |
|---|---|---|
| Snap (snapcraft.io) | `snap/snapcraft.yaml` | Run `snapcraft register coremetrics` once, then `snapcraft upload --release=stable` from CI. Needs Ubuntu One account on first run. |
| Flatpak (Flathub) | `packaging/flatpak/io.github.sviatil0.CoreMetrics.yml` | One-time submission PR to [flathub/flathub](https://github.com/flathub/flathub). Replace `REPLACE_WITH_*_SHA256` placeholders with the real SDL3 tarball hashes Flathub's CI computes during build. After acceptance, every push to `main` of the Flathub repo rebuilds. |

## Gated (manual review)

| Channel | Where | Blocker |
|---|---|---|
| `homebrew/homebrew-core` | upstream Homebrew | Acceptance criteria typically: 30+ stars, multi-release history, no overlap with an existing formula. Custom tap is the right channel until traction. |
| Debian apt | `bookworm-backports` etc | Needs Debian Maintainer status + ITP bug + sponsor. Realistic only after Snap / Flatpak have been live for months. |
| Mac App Store | App Store Connect | **Architecturally blocked**: the App Sandbox denies `proc_listpids` and no entitlement unlocks it (see Apple Developer Forums thread 691857). Shipping on the MAS would require splitting the per-process backend into a privileged XPC helper, the same pattern iStat Menus uses. Out of scope for this project. |
| Developer ID + notarization (NOT App Store) | Direct download | Needs Apple Developer Program enrollment ($99/yr). Workflow stub lives in `.github/workflows/codesign-mac.yml` and runs only when the `APPLE_*` secrets are present in the repo settings. |
| Windows Chocolatey | community.chocolatey.org | Account + manual moderation queue. Realistic after the v0.2.x line has been stable. |
| Arch AUR | aur.archlinux.org | Maintainer-account + SSH key needed on first push. PKGBUILD is small and can be added when desired. |

## Adding a new channel

For each new channel:

1. Add the manifest/recipe under `packaging/<channel>/`.
2. Add a build job to `.github/workflows/release.yml` that produces and attaches the artifact.
3. Document the publish step in this file under either Staged or Gated.

The principle: every artifact is built by CI off the same tag, no per-channel local toolchains.
