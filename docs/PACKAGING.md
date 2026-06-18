# Packaging

CoreMetrics ships to every major package manager. This document is the
single source of truth for what each channel looks like, whether it's
automated by CI or manually bumped, and what to do at release time.

## At a glance

| Channel | Audience | Manifest location | Bump trigger | Status |
|---|---|---|---|---|
| GitHub Releases (`tar.gz`) | All POSIX | built in CI | `git push tag v*` | Automated |
| GitHub Releases (`.deb`) | Debian / Ubuntu | built in CI | `git push tag v*` | Automated |
| GitHub Releases (`.rpm`) | Fedora / RHEL / openSUSE | built in CI via `fpm` | `git push tag v*` | Automated |
| GitHub Releases (`.zip`, Windows) | Windows | built in CI | `git push tag v*` | Automated |
| Homebrew tap | macOS / Linux | `sviatil0/homebrew-coremetrics` | `brew-tap-publish.yml` opens PR | Automated |
| Scoop | Windows | `packaging/scoop/coremetrics.json` | Maintainer pushes to a bucket | Manual bump |
| winget | Windows | `packaging/winget/*.yaml` | Maintainer opens PR to `microsoft/winget-pkgs` | Manual bump |
| Chocolatey | Windows | `packaging/chocolatey/` | Maintainer pushes `.nupkg` to community.chocolatey.org | Manual bump |
| AUR | Arch | `packaging/aur/PKGBUILD` + `.SRCINFO` | Maintainer pushes to `aur.archlinux.org/coremetrics-bin` | Manual bump |
| MacPorts | macOS | `packaging/macports/Portfile` | Maintainer opens PR to `macports/macports-ports` | Manual bump |
| Nix | NixOS / Nix | `packaging/nix/{default,flake}.nix` | Maintainer opens PR to `NixOS/nixpkgs` | Manual bump |
| Snap | Ubuntu / desktop Linux | `packaging/snap/snapcraft.yaml`, `snap/snapcraft.yaml` | `snapcraft upload` | Manual bump |
| Flatpak (Flathub) | Desktop Linux | `packaging/flatpak/io.github.sviatil0.coremetrics.yaml` | Push to Flathub repo | Manual bump after first-submit |
| RPM source (Fedora COPR) | Fedora | `packaging/rpm/coremetrics.spec` | Maintainer uploads SRPM | Manual bump |

## Automated channels

These run end-to-end from a single `git push origin vX.Y.Z`:

### GitHub Releases (multi-arch tarballs, `.deb`, `.rpm`, Windows `.zip`)

The `release` workflow (`.github/workflows/release.yml`) fans out across
Linux, macOS, and Windows runners. Each runner:

- builds `bin/coremetrics`,
- packages a `tar.gz` (POSIX) or `zip` (Windows),
- runs `fpm -s dir -t deb` (Linux) and `fpm -s dir -t rpm` (Linux) to
  produce the `.deb` and `.rpm`,
- uploads everything to the GitHub Release for the tag.

No human action required between `git tag v0.2.18 && git push --tags`
and the GitHub Release being live.

### Homebrew tap

`.github/workflows/brew-tap-publish.yml` watches the `Release` workflow.
On success it downloads both tarballs, computes their sha256, rewrites
`Formula/coremetrics.rb` in `sviatil0/homebrew-coremetrics`, and opens a
PR. Merge the PR (or enable auto-merge on the tap) and the formula is
live for `brew install sviatil0/coremetrics/coremetrics`.

## Manual bump channels

For every other channel the manifest is checked in and bumped per release
by the maintainer. Each section below names the file to edit and the
upstream submit step.

### Scoop (Windows)

1. Wait for the Windows `.zip` artifact to land on the GitHub Release.
2. Compute `sha256sum coremetrics-vX.Y.Z-windows-x86_64.zip`.
3. Edit `packaging/scoop/coremetrics.json`: bump `version`, `url`, and
   `hash`. The `autoupdate` block lets a future scoop bucket auto-bump
   on its own once the URL pattern is stable.
4. Publish via a scoop bucket: either push to
   `sviatil0/scoop-coremetrics` and tell users
   `scoop bucket add coremetrics https://github.com/sviatil0/scoop-coremetrics`,
   or submit to the community `extras` bucket.

### winget (Windows)

1. Wait for the Windows `.zip` artifact to land on the GitHub Release.
2. Bump the three files under `packaging/winget/`:
   - `sviatil0.coremetrics.yaml` (version pointer),
   - `sviatil0.coremetrics.installer.yaml` (URL + sha256),
   - `sviatil0.coremetrics.locale.en-US.yaml` (`PackageVersion`, release
     notes URL).
3. Validate with `winget validate --manifest packaging/winget`.
4. Open a PR to `microsoft/winget-pkgs` under
   `manifests/s/sviatil0/coremetrics/<version>/`. The Microsoft bots run
   automated checks and gate on a `wingetbot run` comment from a moderator.

### Chocolatey (Windows)

1. Wait for the Windows `.zip` artifact to land on the GitHub Release.
2. Edit `packaging/chocolatey/coremetrics.nuspec` (`<version>`,
   `<releaseNotes>`) and `packaging/chocolatey/tools/chocolateyinstall.ps1`
   (`$version`, `$url`, `$checksum`).
3. From a Windows host: `choco pack packaging/chocolatey/coremetrics.nuspec`
   then `choco push coremetrics.<version>.nupkg --source community`.
4. The Chocolatey moderation queue can take up to two weeks on first push.

### AUR (Arch User Repository)

1. Wait for the Linux `tar.gz` to land on the GitHub Release.
2. Edit `packaging/aur/PKGBUILD`: bump `pkgver` and replace
   `sha256sums` with the new tarball's sha256.
3. Regenerate `.SRCINFO`: `makepkg --printsrcinfo > .SRCINFO`.
4. From a checkout of `ssh://aur@aur.archlinux.org/coremetrics-bin.git`,
   commit both files and push. The first push requires an AUR account
   and an uploaded SSH key (one-time).

### MacPorts

1. Wait for the source tarball that GitHub publishes for the tag
   (`https://github.com/sviatil0/coremetrics/archive/refs/tags/vX.Y.Z.tar.gz`).
2. Edit `packaging/macports/Portfile`: bump `version` and the three
   `checksums` (`rmd160`, `sha256`, `size`). MacPorts provides
   `port checksum coremetrics` to recompute all three locally.
3. Open a PR to `macports/macports-ports` under
   `aqua/coremetrics/Portfile`. First submission requires a maintainer
   ack from one of the existing macports committers.

### Nix

1. Compute the source hash:
   `nix-prefetch-url --unpack https://github.com/sviatil0/coremetrics/archive/vX.Y.Z.tar.gz`.
2. Replace `REPLACE_WITH_NIX_SRC_HASH` in `packaging/nix/default.nix`
   with the new hash and bump `version`.
3. Open a PR to `NixOS/nixpkgs` adding
   `pkgs/by-name/co/coremetrics/package.nix` that wraps the same
   derivation. For pre-upstream consumption, users can already do:

   ```nix
   { coremetrics = pkgs.callPackage (fetchTarball
       "https://github.com/sviatil0/coremetrics/archive/vX.Y.Z.tar.gz"
       + "/packaging/nix") { }; }
   ```

   Or, with flakes: `nix run github:sviatil0/coremetrics?dir=packaging/nix`.

### Snap

1. Bump `version` in both `snap/snapcraft.yaml` and
   `packaging/snap/snapcraft.yaml` (the root copy is what `snapcraft pack`
   picks up; the `packaging/` copy is a mirror so every channel has one
   home).
2. Build: `snapcraft pack` on an LXD-enabled host produces
   `coremetrics_<version>_<arch>.snap`.
3. Publish: `snapcraft upload --release=stable coremetrics_*.snap`.
4. **First-publish prerequisite**: create an Ubuntu One account and run
   `snapcraft register coremetrics` once. Until that runs, `upload`
   fails with a name-collision error.

### Flatpak (Flathub)

1. Bump `version` and the `releases` block in
   `packaging/flatpak/io.github.sviatil0.coremetrics.appdata.xml`.
2. Replace each `REPLACE_WITH_*_SHA256` placeholder in
   `packaging/flatpak/io.github.sviatil0.coremetrics.yaml` with the
   actual SDL3 tarball hashes. Flathub CI computes and surfaces these
   on the first build, but they must be filled in before the first
   submission.
3. **First-publish prerequisite**: open a "New Submission" issue at
   <https://github.com/flathub/flathub/issues/new?template=new_app.yml>.
   The Flathub team then provisions
   `flathub/io.github.sviatil0.coremetrics` and grants push access.
4. Subsequent releases: push a commit bumping `version` and the
   `releases` block to the per-app repo's `master` branch. Flathub
   rebuilds on every commit.

### RPM (source)

1. Bump `Version:` and add a `%changelog` entry in
   `packaging/rpm/coremetrics.spec`.
2. Build a SRPM: `rpmbuild -bs packaging/rpm/coremetrics.spec`.
3. Upload to Fedora COPR via `copr-cli build sviatil0/coremetrics <srpm>`.

   Note: the binary `.rpm` shipped on the GitHub Release is produced by
   the `release.yml` workflow via `fpm` and does *not* consume this spec.
   The spec exists for distros that build from source (Fedora COPR,
   openSUSE OBS, RHEL EPEL).

## Adding a new channel

1. Add the manifest/recipe under `packaging/<channel>/`.
2. If the artifact can be built in CI off the source tarball, add a
   build job to `.github/workflows/release.yml`.
3. Document the bump step here, in the table at the top **and** in a
   per-channel subsection below.
4. Update the README's *Install* section with the user-facing install
   command.

## Channels intentionally not covered

| Channel | Why |
|---|---|
| Mac App Store | The App Sandbox denies `proc_listpids` and no entitlement unlocks it. Per-process metrics would have to move into a privileged XPC helper. Architectural blocker; out of scope. |
| Debian apt repository | Requires Debian Maintainer status, an ITP bug, and a sponsor. Realistic only after we have a multi-year release history. Until then the `.deb` on the GitHub Release is the install path. |
| `homebrew/homebrew-core` | Upstream Homebrew typically wants 30+ stars and multi-release history before accepting a formula. The custom tap is the right channel until then. |
