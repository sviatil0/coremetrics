# Packaging

CoreMetrics ships to every major package manager. Each subdirectory holds
the manifest or recipe for one channel; the full release matrix and bump
procedure lives in [`docs/PACKAGING.md`](../docs/PACKAGING.md).

## Layout

| Channel | Directory |
|---|---|
| AUR (Arch User Repository) | `aur/` |
| Chocolatey (Windows) | `chocolatey/` |
| Flatpak (Flathub) | `flatpak/` |
| MacPorts | `macports/` |
| Nix | `nix/` |
| RPM (source spec) | `rpm/` |
| Scoop (Windows) | `scoop/` |
| Snap (mirror of `snap/snapcraft.yaml`) | `snap/` |
| winget (Windows) | `winget/` |

Homebrew, the `.deb`, the `.rpm`, the POSIX tarballs, and the Windows
`.zip` are produced by `.github/workflows/release.yml` on every `v*` tag.
The Homebrew tap is updated by `.github/workflows/brew-tap-publish.yml`.

## Bump procedure

For each release see [`docs/PACKAGING.md`](../docs/PACKAGING.md) for the
per-channel checklist. Channels marked there as *Automated* require no
human action; channels marked *Manual bump* need the maintainer to bump
the version, replace the SHA placeholder, and push to the channel's
upstream.
