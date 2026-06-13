$ErrorActionPreference = 'Stop'

# Chocolatey install script for coremetrics. Downloads the Windows release
# zip from the GitHub Release page and unpacks it into the package's tools
# directory; Chocolatey then puts the binary on PATH via its shim system.
#
# This package targets the v0.1.1 line. When upstream cuts a new tag, the
# maintainer bumps $version + $checksum and reuploads to community.chocolatey.org.

$packageName = 'coremetrics'
$version     = '0.1.1'
$toolsDir    = "$(Split-Path -Parent $MyInvocation.MyCommand.Definition)"

# The Windows release artifact will be produced by the upstream release
# workflow once the windows-latest build leg is gated (today it ships only
# Linux and macOS tarballs + a .deb). When that lands, replace these two
# fields with the real values from the GitHub Release; until then this
# script will fail fast at install time, which is the correct behavior.
$url         = "https://github.com/sviatil0/coremetrics/releases/download/v$version/coremetrics-v$version-windows-x86_64.zip"
$checksum    = 'REPLACE_WITH_WINDOWS_ZIP_SHA256'
$checksumType = 'sha256'

Install-ChocolateyZipPackage `
    -PackageName $packageName `
    -Url64 $url `
    -UnzipLocation $toolsDir `
    -Checksum64 $checksum `
    -ChecksumType64 $checksumType

# Chocolatey auto-shims any .exe under $toolsDir. base.xml + assets/ ship
# alongside the binary inside the zip, so AssetPath::resolve() finds them
# via the SDL_GetBasePath fallback to <exe>/../share/coremetrics/.
