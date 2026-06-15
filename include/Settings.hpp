#ifndef __SETTINGS_HPP__
#define __SETTINGS_HPP__

#include <cstdint>

// Tiny disk-backed preferences for CoreMetrics. The live UI keeps four
// pieces of state worth surviving a relaunch: the poll cadence, whether
// sparklines are drawn over the System tab, and the active sort column +
// direction on the Processes tab. Anything else (window size, mute,
// filter, tree mode) is intentionally session-only.
//
// File layout is a single flat key=value text file written line per key.
// Unknown keys are tolerated; malformed lines are skipped. Missing values
// leave the in-memory globals untouched so CLI flags and compiled defaults
// remain authoritative when no config exists.
//
// Path:
//   * macOS / Linux: $HOME/.config/coremetrics/config
//   * Windows:       %APPDATA%/coremetrics/config
//
// load() is called once at startup, before CLI parsing, so the argv
// pass can override saved values. save() is called once on clean exit
// right before main returns 0, so a Ctrl-C-killed run does not clobber
// the file with a half-initialized state.
//
// The globals being persisted live as file-scope statics in
// coremetrics.cpp; the read/write API takes references so we never have
// to widen their linkage just to support disk persistence.
namespace Settings
{
    // Read the config file if it exists and copy any recognized keys
    // into the supplied outputs. Returns true if a file was read
    // end-to-end (even if some lines were skipped), false if the file
    // did not exist or could not be opened. Errors are not surfaced to
    // the user: a fresh install simply has no file yet. Each output is
    // left untouched when its key is missing or its value is malformed,
    // so CLI flags applied afterward still win over saved values.
    bool load(std::uint64_t &pollIntervalMs,
              bool &sparklinesEnabled,
              int &sortColumn,
              bool &sortAscending);

    // Write the current values to disk, creating the parent directory
    // tree if it does not exist. Returns true on a clean write, false
    // if the directory could not be created or the file could not be
    // opened. The caller does not treat this as fatal because losing
    // prefs is strictly better than crashing on exit.
    bool save(std::uint64_t pollIntervalMs,
              bool sparklinesEnabled,
              int sortColumn,
              bool sortAscending);
}

#endif
