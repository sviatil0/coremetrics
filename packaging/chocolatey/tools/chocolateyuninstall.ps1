$ErrorActionPreference = 'Stop'

# Chocolatey clears the package directory automatically on uninstall, so
# this script just stops any running instance of the binary first so the
# uninstall does not race with a live process.

$processName = 'coremetrics'
Get-Process -Name $processName -ErrorAction SilentlyContinue | ForEach-Object {
    Stop-Process -Id $_.Id -Force
}
