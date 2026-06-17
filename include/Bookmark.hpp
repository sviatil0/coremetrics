#ifndef __BOOKMARK_HPP__
#define __BOOKMARK_HPP__

#include <string>
#include <unordered_set>

// Persistent "starred" process list. The Processes tab will eventually
// let the user pin a process so it stays visible (or surfaces at the
// top) across restarts. This module is the storage layer for that
// feature: a simple, hand-editable flat file under the per-user
// CoreMetrics config directory.
//
// Path:
//   POSIX:    $HOME/.config/coremetrics/bookmarks
//   Windows:  %APPDATA%\coremetrics\bookmarks
//
// On-disk record format (one entry per line, newline-terminated):
//   firefox|1234
//   nginx
//
// Two flavours are accepted on load and emitted on save:
//   1. `name|pid` for an exact pid+name pin. Useful while the process is
//      still alive and the user wants to re-bind to that specific
//      instance.
//   2. `name` alone for a wildcard match. This survives a pid rotation
//      (the user really wanted to track "firefox", not pid 1234) and is
//      the form the UI will default to.
//
// The in-memory representation is the verbatim line for both flavours
// (i.e. `"firefox|1234"` or `"firefox"`) so callers can do an O(1)
// `count()` on either form without re-parsing.
//
// All I/O is best-effort: a missing file, an unreadable directory, or a
// malformed line is silently skipped so the live UI never has to handle
// a thrown filesystem error. Both functions return `true` on success,
// `false` only on a hard failure that prevented even partial work.
namespace Bookmark
{
    // Read bookmarks from `path` into `out`. The destination is cleared
    // first so a successful load reflects the file's current contents.
    // Returns `false` when the file does not exist or cannot be opened;
    // returns `true` after a successful read, including the trivial
    // empty-file case. Malformed lines (empty, leading `|`, embedded
    // newlines from a corrupt write) are silently skipped.
    bool load(const std::string &path,
              std::unordered_set<std::string> &out);

    // Write `in` to `path`, creating the parent directory if needed.
    // The output is sorted for a stable on-disk diff. An empty set
    // produces an empty file rather than deleting the path so callers
    // can tell "user has no bookmarks" from "config dir is missing".
    // Returns `false` only on a hard I/O failure.
    bool save(const std::string &path,
              const std::unordered_set<std::string> &in);

    // Canonical bookmarks path for the current host. Returns an empty
    // string when the relevant env var (`HOME` on POSIX, `APPDATA` on
    // Windows) is missing so the caller can bail without ever touching
    // the filesystem.
    std::string defaultPath();
}

#endif
