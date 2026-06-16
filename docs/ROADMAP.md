# CoreMetrics Roadmap

Post Neo Scholars submission roadmap. Honest about what is already shipped versus what is planned. Dates are aspirational, scope is not a promise.

## Done (v0.2.x line)

The v0.2.x release line covers everything shipped between the Neo Scholars submission tag and the current `dev` head.

- htop-comparable Processes tab with column headers, hover highlight, and sort glyph
- Per-core CPU strip alongside the aggregate CPU readout
- Memory breakdown panel (used, cached, free, swap)
- Sparklines for CPU, RAM, GPU, and NET with configurable history depth
- Process tree mode with per-pid collapse on space, persisted across runs
- Process filter box and incremental match highlighting
- Kill menu with confirmation prompt and signal selection
- Multi-column sort (CPU, MEM, PID, name) with persisted last choice
- CSV and JSON export of the live snapshot
- Headless `--screenshot` path that skips SDL audio init
- Threshold-based color stroking on sparklines via `setThresholdMode`
- Bold readout text for headline numbers
- Summary row above the Processes column headers (totals across all pids)
- Contribution badge and per-author breakdown refreshed from git blame
- `coremetrics(1)` manpage
- Bash, zsh, and fish shell completions
- Settings persistence under `~/.config/coremetrics/config` (poll-ms, sparklines, sort, collapsed pids)
- Kill audit log at `~/.config/coremetrics/kill.log`
- `Theme::` palette getters adopted across the UI layer
- `SECURITY.md` disclosure policy
- Dependabot weekly updates for GitHub Actions
- Stale issue and PR bot
- Code coverage CI via lcov plus Codecov upload
- `actionlint` workflow validation, concurrency groups, pre-commit hook, `.clang-format`
- `CHANGELOG.md` generator script and `CITATION.cff`
- GitHub Sponsors button
- Codespaces devcontainer and issue templates
- Arch Linux `coremetrics-bin` PKGBUILD
- Windows per-process disk-IO via `GetProcessIoCounters`
- Windows per-core CPU sampling

## v0.3 next (planned, scoped)

Items on this list are committed for the v0.3 line. Each one has a clear acceptance criterion and a single owning area of code.

- Light theme via `Theme::set("light")`, switchable at runtime through the existing palette getters
- Window resize support: respond to SDL resize events and re-layout panels rather than clamping to the launch size
- System info header: CPU model, OS name and version, hostname, and uptime above the readouts
- Battery state on laptops: percentage, charging flag, and time-to-empty estimate where the OS exposes it
- Notifications when a threshold is crossed: opt-in desktop notification when CPU, RAM, or GPU stays above the configured threshold for N consecutive samples
- NET sparkline auto-scale: dynamic Y-axis on the network sparkline instead of a fixed ceiling

## v0.4 considered (no committed timeline)

Items here are on the radar but not scheduled. They may grow, shrink, or get dropped.

- GPU memory percentage per process via Vulkan and Metal queries
- Per-process network I/O through per-pid socket accounting on Linux
- Remote monitoring over a Unix domain socket so a second instance can attach to a running daemon
- Self-update flow that downloads a signed release tarball and swaps the binary in place

## Out of scope

These are explicit nos. Filing an issue asking for them will get a polite link back here.

- Windowing toolkit abstraction (no Qt, no GTK, no wxWidgets layer over SDL)
- Embedded scripting language (no Lua, no Python, no JS hooks)
- Web frontend (no HTTP server, no browser UI)

## How to influence the roadmap

Open an issue with the `roadmap` label. Include the use case, not just the feature name. PRs that implement items from v0.3 are welcome at any time. PRs that implement items from v0.4 are welcome but please open a discussion first so the design lands cleanly.
