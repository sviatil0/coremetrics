#ifndef __EXPORTER_HPP__
#define __EXPORTER_HPP__

#include <string>

// One-shot exporters for the CoreMetrics live state. Both writers take
// a single fresh sample of the same aggregates the live UI reads (CPU%,
// MEM%, GPU%, uptime, disk usage, net rx/tx) plus the top-N process
// list, then emit a flat file at `path` and return. They are designed
// to run from the CLI flag handlers in coremetrics.cpp before SDL_Init,
// so they never touch a window or surface.
//
// On any I/O failure (cannot open the path, write fails, etc.) the
// caller treats the return value as failure: false = something went
// wrong, true = file written end-to-end.
namespace Exporter
{
    // CSV layout (one row per record, comma-separated, no quoting since
    // we control every field):
    //   row 0:  "aggregate", CPU%, MEM%, GPU%, uptime_seconds,
    //           disk_used_pct, net_rx_kb_s, net_tx_kb_s
    //   row 1:  header for the per-process rows
    //   row 2+: pid, parent_pid, name, cpu_pct, mem_pct,
    //           read_kb_s, write_kb_s
    bool writeCsv(const std::string &path);

    // JSON layout: { "aggregate": { ... }, "processes": [ { ... } ] }
    // Hand-rolled to avoid adding a dependency. Field names mirror the
    // CSV columns.
    bool writeJson(const std::string &path);
}

#endif
