# Windows ARM64 smoke build + run for CoreMetrics.
#
# Paste into an elevated PowerShell once Win11 ARM has finished installing.
# Installs MSYS2 clangarm64 toolchain + SDL3 from MSYS2 packages, clones
# the latest main, builds bin/coremetrics, runs a 5-second headless
# screenshot, and prints a clear PASS / FAIL summary.
#
# Total runtime on a fresh VM: ~10-20 minutes (dominated by MSYS2 install
# and the first pacman -Syu).

$ErrorActionPreference = 'Stop'

function Step($msg) { Write-Host "==> $msg" -ForegroundColor Cyan }

Step "Downloading MSYS2 installer"
$msysExe = "$env:USERPROFILE\Downloads\msys2-x86_64-latest.sfx.exe"
if (-not (Test-Path $msysExe)) {
    Invoke-WebRequest `
        -Uri 'https://github.com/msys2/msys2-installer/releases/latest/download/msys2-base-x86_64-latest.sfx.exe' `
        -OutFile $msysExe
}

Step "Extracting MSYS2 to C:\msys64"
if (-not (Test-Path 'C:\msys64\usr\bin\bash.exe')) {
    Start-Process -Wait -FilePath $msysExe -ArgumentList '-y', '-oC:\'
}

$bash = 'C:\msys64\usr\bin\bash.exe'

Step "Updating MSYS2 package database (may take several minutes)"
& $bash -lc 'pacman -Syu --noconfirm'
# Second pass: the first pacman -Syu can ask to close the shell; doing it
# twice guarantees a stable base.
& $bash -lc 'pacman -Syu --noconfirm'

Step "Installing clangarm64 toolchain + SDL3 trio + git"
& $bash -lc 'pacman -S --noconfirm --needed git make mingw-w64-clang-aarch64-clang mingw-w64-clang-aarch64-pkgconf mingw-w64-clang-aarch64-SDL3 mingw-w64-clang-aarch64-SDL3_ttf mingw-w64-clang-aarch64-SDL3_image'

Step "Cloning sviatil0/coremetrics into C:\\coremetrics"
if (-not (Test-Path 'C:\coremetrics')) {
    & $bash -lc 'git clone https://github.com/sviatil0/coremetrics.git /c/coremetrics'
}

Step "Building bin/coremetrics under clangarm64"
$buildCmd = @'
set -e
export PATH=/clangarm64/bin:$PATH
cd /c/coremetrics
mkdir -p obj bin
SDL_CFLAGS=$(pkg-config --cflags sdl3 sdl3-ttf sdl3-image)
SDL_LIBS=$(pkg-config --libs sdl3 sdl3-ttf sdl3-image)
make CXX=clang++ \
     bin/coremetrics \
     CXXFLAGS="-std=c++23 -Wall -I ./include $SDL_CFLAGS" \
     LDFLAGS="$SDL_LIBS"
'@
& $bash -lc $buildCmd

Step "Running headless 5-second smoke test + saving screenshot"
$runCmd = @'
set -e
cd /c/coremetrics
export PATH=/clangarm64/bin:$PATH
./bin/coremetrics --screenshot /c/coremetrics-win.png --duration 5
'@
& $bash -lc $runCmd

if (Test-Path 'C:\coremetrics-win.png') {
    Step "PASS: smoke screenshot saved at C:\\coremetrics-win.png"
    Write-Host ""
    Write-Host "Pull it from the host via:" -ForegroundColor Yellow
    Write-Host "    utmctl file pull Win11 C:\\coremetrics-win.png /tmp/coremetrics-win.png"
} else {
    Step "FAIL: screenshot was not created"
    exit 1
}
