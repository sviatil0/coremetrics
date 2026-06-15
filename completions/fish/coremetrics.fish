# fish completion for the coremetrics CLI.
#
# Install:
#   - User-local:   cp completions/fish/coremetrics.fish ~/.config/fish/completions/
#   - System-wide:  sudo cp completions/fish/coremetrics.fish /usr/share/fish/vendor_completions.d/
#
# Keep the flag list in sync with the argv parsing block in coremetrics.cpp.

# Boolean flags: no value follows.
complete -c coremetrics -l sparklines -d 'Enable sparkline graphs in the live UI'
complete -c coremetrics -l tree       -d 'Open Processes tab in parent/child tree mode'

# Flags that take a free-form value: suppress file completion so the
# shell does not propose paths where a substring or number is expected.
complete -c coremetrics -l filter   -r -f -d 'Seed Processes-tab name filter (substring)'
complete -c coremetrics -l poll-ms  -r -f -d 'Override default 500ms poll cadence'
complete -c coremetrics -l duration -r -f -d 'Run for N seconds then exit'

# Flags that take a filesystem path: enable file completion (-F).
complete -c coremetrics -l screenshot  -r -F -d 'Render one frame, write PNG to path, exit'
complete -c coremetrics -l export-csv  -r -F -d 'Write one-shot CSV snapshot to path, exit'
complete -c coremetrics -l export-json -r -F -d 'Write one-shot JSON snapshot to path, exit'
