# Changelog

Generated from git tags via `scripts/gen-changelog.sh`. Do not hand-edit;
rerun the script to refresh.

## v0.2.18 - 2026-06-15

### Features
- feat(processes): summary row with totals
- feat(processes): per-pid tree collapse on space
- feat(completions): bash/zsh/fish completion scripts

### Fixes
- fix(processes): move summary strip above column headers

### Documentation
- docs(man): add coremetrics(1) manpage
- docs: add COMPARISON.md vs htop / btop / Activity Monitor

### CI
- ci: code coverage via lcov + codecov upload

### Chores
- chore: enable GitHub Sponsors button and README badge

### Release
- release: v0.2.18 footer version

## v0.2.17 - 2026-06-15

### Features
- feat(ui): help overlay toggled by ? key
- feat(ui): network rx/tx sparkline below CPU/RAM/GPU
- feat(settings): persist poll-ms/sparklines/sort to ~/.config/coremetrics/config
- feat(win): per-process disk-IO via GetProcessIoCounters

### Fixes
- fix(screenshot): skip SDL audio init in headless --screenshot path

### Benchmarks
- bench(render): drawPixel, drawText, Label::draw rates

### Packaging
- packaging(aur): add coremetrics-bin PKGBUILD for Arch Linux

### Chores
- chore: codespaces devcontainer + GitHub issue templates

### Release
- release: v0.2.17 footer version

## v0.2.16 - 2026-06-15

### Features
- feat(ui): bold readout text for CPU/RAM/GPU percent values
- feat(processes): sort-direction glyph and selected-row highlight
- feat(win): PDH per-core CPU counters
- feat(export): --export-csv and --export-json flags
- feat(linux): full /proc/meminfo breakdown parse
- feat(sparkline): 50% midline and axis ticks

### Fixes
- fix(exporter): defend CSV against formula injection (CWE-1236)

### Documentation
- docs: kill em-dashes/en-dashes, fix install snippet, harden CI claim and hook

### Tests
- test(process-utils): cover negative/NaN/wrap/boundary edge cases

### Benchmarks
- bench(ui): sparkline and bar render rates

### CI
- ci: headless screenshot smoke on all three OS legs

### Release
- release: v0.2.16 footer version

## v0.2.15 - 2026-06-15

### Features
- feat(ui): sparkline fill, bar depth shading, threshold palette hoist

### Fixes
- fix(metrics): clamp proc_listpids count, route per-process IO via tested helper
- fix(ui): footer text overflow + sparkline label collision

### Release
- release: v0.2.15 footer version

## v0.2.14 - 2026-06-15

### Chores
- chore(contribution): refresh badge and per-author breakdown from git blame

### Release
- release: v0.2.14 footer version

## v0.2.13 - 2026-06-15

### Features
- feat(ui): threshold-based bar coloring + matching percent text

### Release
- release: v0.2.13 footer version

## v0.2.12 - 2026-06-15

### Fixes
- fix(ui): declutter top of System tab + label cores strip

### Chores
- chore(contribution): refresh badge and per-author breakdown from git blame

### Release
- release: v0.2.12 footer version

## v0.2.11 - 2026-06-15

### Performance
- perf(mac): cache HW_MEMSIZE + page_size in mem readers
- perf(font): cache TTF render surfaces keyed on (text, rgb)

### Documentation
- docs(readme): front-load value prop + add 'Where to start reading' nav

### Chores
- chore(contribution): refresh badge and per-author breakdown from git blame

### Release
- release: bump to v0.2.11 + regen System hero
- release: bump to v0.2.10 + regen System hero

## v0.2.9 - 2026-06-15

### Features
- feat(metrics,ui): network I/O backend + aggregate-I/O strip layout

### Fixes
- fix(win): revert to GetIfTable; MinGW headers don't expose GetIfTable2
- fix(win): define _WIN32_WINNT=0x0601 before windows.h so GetIfTable2 resolves
- fix(win,perf): include netioapi.h + drawBox fast path for small boxes
- fix(win): use GetIfTable2 to avoid 32-bit counter wrap

### Chores
- chore(contribution): refresh badge and per-author breakdown from git blame
- chore: drop dead code + bump drawBox threshold + add sstream include
- chore(contribution): refresh badge and per-author breakdown from git blame
- chore(contribution): refresh badge and per-author breakdown from git blame
- chore(contribution): refresh badge and per-author breakdown from git blame
- chore(contribution): refresh badge and per-author breakdown from git blame
- chore(contribution): refresh badge and per-author breakdown from git blame
- chore(contribution): refresh badge and per-author breakdown from git blame
- chore(contribution): refresh badge and per-author breakdown from git blame

### Release
- release: bump to v0.2.9 + regen System hero
- release: bump to v0.2.8 + regen heros

## v0.2.8 - 2026-06-15

### Chores
- chore(contribution): refresh badge and per-author breakdown from git blame

## v0.2.7 - 2026-06-15

### Features
- feat(ui): aggregate disk I/O strip on System tab

### Chores
- chore(contribution): refresh badge and per-author breakdown from git blame

### Release
- release: bump to v0.2.7 + regen System hero

## v0.2.6 - 2026-06-15

### Features
- feat(ui,cli): --poll-ms flag + EXIT redesign + drop cores label

### Fixes
- fix: audit roundup from 4 parallel review agents
- fix(ui): redesign EXIT button + regen processes hero

### Chores
- chore(contribution): refresh badge and per-author breakdown from git blame

### Release
- release: bump to v0.2.6 + regen heros

## v0.2.5 - 2026-06-14

### Fixes
- fix(ui): declutter mem breakdown strip + label per-core strip

### Chores
- chore(contribution): refresh badge and per-author breakdown from git blame

### Release
- release: bump to v0.2.5 + regen System hero

## v0.2.4 - 2026-06-14

### Features
- feat(scripts): Windows ARM64 smoke build + run PowerShell

### Fixes
- fix(ui): align Load text to the y=44 status row baseline

### Documentation
- docs: align README and API.md with v0.2.1 / v0.2.2 / v0.2.3

### CI
- ci(windows): flip allow-failure to false now that the leg is stable
- ci(windows): -static-libgcc -static-libstdc++
- ci(windows): run bin/tests.exe (Windows binary suffix)
- ci(windows): also mkdir obj/ before building
- ci(windows): copy SDL3 DLLs next to bin/tests before running
- ci(windows): add SDL3 lib dirs to PATH so tests can find SDL3.dll
- ci(windows): link against -lpdh -lpsapi
- ci(windows): bypass pkg-config, use SDL3_ROOT env vars directly

### Chores
- chore(contribution): refresh badge and per-author breakdown from git blame

### Release
- release: v0.2.4 + bug fixes + helper extraction + tests

## v0.2.3 - 2026-06-13

### Features
- feat(processes,ui): per-process DISK I/O column

### Chores
- chore(contribution): refresh badge and per-author breakdown from git blame

### Release
- release: bump to v0.2.3 + regen hero screenshots

## v0.2.2 - 2026-06-13

### Features
- feat(metrics,ui): root volume disk usage on the System tab

### Chores
- chore(contribution): refresh badge and per-author breakdown from git blame

### Release
- release: bump to v0.2.2 + regen hero screenshots

## v0.2.1 - 2026-06-13

### Features
- feat(ui): live process-count footer + sparkline strip labels

### Fixes
- fix: cross-platform metric backend roundup

### Documentation
- docs(readme): align user-facing capabilities, screenshot caption, install path, test count

### Tests
- test(processutils): pure helpers for io rate, filter, uptime, sort default

### Chores
- chore(contribution): refresh badge and per-author breakdown from git blame

### Release
- release: bump to v0.2.1 + regen hero screenshots

## v0.2.0 - 2026-06-13

### Features
- feat(processes): per-process disk I/O rate (read/write KB/sec)
- feat(cli): --tree and --filter flags for screenshot capture
- feat(processes,ui): process tree view ('t' toggle, htop F5)
- feat(processes,ui): '/' filter mode for the Processes tab (htop F3)
- feat(signals): Windows backend via TerminateProcess + DebugBreakProcess
- feat(metrics,ui): uptime + 1/5/15-min load averages in System header
- feat(metrics,ui): segmented memory breakdown bar (htop feature 2)
- feat(packaging): AUR PKGBUILD + Chocolatey nuspec stubs
- feat(release): Developer ID codesign + notarization pipeline
- feat(packaging): add Snap + Flatpak manifests + channel status doc
- feat(processes,signals): htop-style row selection + signal menu + kill
- feat(metrics,ui): per-logical-CPU bars (htop feature 1)
- feat(ui,docs): modernize chrome and make the contribution section live

### Fixes
- fix(tests): SignalUtils numeric assertion was mac-specific

### Documentation
- docs(readme): document htop-style controls on the Processes tab
- docs(readme): add install-path badges (Homebrew, .deb, downloads)
- docs: ADVANTAGES.md explaining the cross-platform + custom-GUI moat

### CI
- ci: add Windows build leg (non-blocking) to compile-check the Win backend

### Chores
- chore(screenshots): regen hero with tree view + tree-mode fetch boost
- chore(docs): regen hero screenshots with uptime + load + mem breakdown
- chore(docs): regen hero screenshots with memory breakdown strip visible
- chore(docs): regen hero screenshots with per-core CPU strip

### Release
- release: bump to v0.2.0

