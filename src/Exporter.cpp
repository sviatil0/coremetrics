#include "Exporter.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "SystemMetrics.hpp"

namespace
{
    // How many top processes to sample for the export. Mirrors the
    // value the live Processes tab uses for the visible window so the
    // exported file matches what the user sees.
    constexpr std::size_t EXPORT_TOP_N = 20;

    // Float formatting precision for all percentage / rate fields.
    // Two decimals is what the on-screen readouts render and is also
    // plenty for CSV/JSON consumers.
    constexpr int FLOAT_PRECISION = 2;

    // Disk-used-percent guard: a totalKb of 0 means readDiskUsage()
    // failed on this platform, in which case the exported value is 0
    // rather than NaN from division by zero.
    constexpr unsigned long long DISK_TOTAL_MIN = 1ULL;

    // CSV field separator and JSON indentation step are named so the
    // file format is obvious at the call site (and a single edit can
    // change the dialect later if needed).
    const std::string CSV_SEP = ",";
    const std::string JSON_INDENT = "  ";

    std::string formatFloat(float value)
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(FLOAT_PRECISION) << value;
        return oss.str();
    }

    // CSV-quote a string field. Per RFC 4180: wrap in double quotes,
    // and double any embedded double-quote. The aggregate / pid / rate
    // fields are numeric so they never go through this path; only the
    // process name field does.
    //
    // Defends against CSV formula injection (OWASP / CWE-1236): if the
    // raw name begins with a character a spreadsheet would interpret as
    // a formula trigger (`=`, `+`, `-`, `@`, tab, carriage return) the
    // function emits a leading single quote inside the quoted field so
    // Excel / Numbers / LibreOffice render the cell as literal text
    // instead of evaluating it. A maliciously named process such as
    // `=cmd|'/c calc'!A0` cannot pivot through a downstream CSV viewer.
    std::string csvQuote(const std::string &in)
    {
        std::string out;
        out.reserve(in.size() + 3);
        out.push_back('"');
        if (!in.empty())
        {
            char first = in.front();
            if (first == '=' || first == '+' || first == '-'
                || first == '@' || first == '\t' || first == '\r')
            {
                out.push_back('\'');
            }
        }
        for (char c : in)
        {
            if (c == '"')
            {
                out.push_back('"');
                out.push_back('"');
            }
            else
            {
                out.push_back(c);
            }
        }
        out.push_back('"');
        return out;
    }

    // JSON-escape a string field. Covers the subset that can appear in
    // a process name on the supported platforms: quote, backslash, and
    // any control character below 0x20.
    std::string jsonEscape(const std::string &in)
    {
        std::string out;
        out.reserve(in.size() + 2);
        out.push_back('"');
        for (char c : in)
        {
            unsigned char uc = static_cast<unsigned char>(c);
            if (c == '"')
            {
                out += "\\\"";
            }
            else if (c == '\\')
            {
                out += "\\\\";
            }
            else if (c == '\n')
            {
                out += "\\n";
            }
            else if (c == '\r')
            {
                out += "\\r";
            }
            else if (c == '\t')
            {
                out += "\\t";
            }
            else if (uc < 0x20)
            {
                std::ostringstream oss;
                oss << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                    << static_cast<int>(uc);
                out += oss.str();
            }
            else
            {
                out.push_back(c);
            }
        }
        out.push_back('"');
        return out;
    }

    // Compute the disk-used percentage from a DiskUsage sample without
    // tripping a divide-by-zero when the platform call failed (totalKb
    // == 0). Returns 0.0 in that case so the exported field is still a
    // valid number.
    float diskUsedPercent(unsigned long long totalKb, unsigned long long freeKb)
    {
        if (totalKb < DISK_TOTAL_MIN)
        {
            return 0.0f;
        }
        unsigned long long usedKb = totalKb > freeKb ? totalKb - freeKb : 0ULL;
        return (static_cast<float>(usedKb) / static_cast<float>(totalKb)) * 100.0f;
    }
}

bool Exporter::writeCsv(const std::string &path)
{
    float cpuPct = SystemMetrics::readCpuPercent();
    float memPct = SystemMetrics::readMemPercent();
    float gpuPct = SystemMetrics::readGpuPercent();
    unsigned long long uptimeSeconds = SystemMetrics::readUptimeSeconds();
    DiskUsage disk = SystemMetrics::readDiskUsage();
    NetIo net = SystemMetrics::readNetIo();
    std::vector<ProcessInfo> procs = SystemMetrics::topProcesses(EXPORT_TOP_N);

    std::ofstream out(path);
    if (!out.is_open())
    {
        return false;
    }

    // Aggregate row first so simple `head -1` consumers can grab the
    // headline numbers without parsing the per-process table.
    out << "aggregate" << CSV_SEP
        << formatFloat(cpuPct) << CSV_SEP
        << formatFloat(memPct) << CSV_SEP
        << formatFloat(gpuPct) << CSV_SEP
        << uptimeSeconds << CSV_SEP
        << formatFloat(diskUsedPercent(disk.totalKb, disk.freeKb)) << CSV_SEP
        << net.rxKbPerSec << CSV_SEP
        << net.txKbPerSec << "\n";

    out << "pid" << CSV_SEP
        << "parent_pid" << CSV_SEP
        << "name" << CSV_SEP
        << "cpu_pct" << CSV_SEP
        << "mem_pct" << CSV_SEP
        << "read_kb_s" << CSV_SEP
        << "write_kb_s" << "\n";

    for (const ProcessInfo &p : procs)
    {
        out << p.pid << CSV_SEP
            << p.parentPid << CSV_SEP
            << csvQuote(p.name) << CSV_SEP
            << formatFloat(p.cpuPct) << CSV_SEP
            << formatFloat(p.memPct) << CSV_SEP
            << p.diskReadKbPerSec << CSV_SEP
            << p.diskWriteKbPerSec << "\n";
    }

    out.flush();
    return static_cast<bool>(out);
}

bool Exporter::writeJson(const std::string &path)
{
    float cpuPct = SystemMetrics::readCpuPercent();
    float memPct = SystemMetrics::readMemPercent();
    float gpuPct = SystemMetrics::readGpuPercent();
    unsigned long long uptimeSeconds = SystemMetrics::readUptimeSeconds();
    DiskUsage disk = SystemMetrics::readDiskUsage();
    NetIo net = SystemMetrics::readNetIo();
    std::vector<ProcessInfo> procs = SystemMetrics::topProcesses(EXPORT_TOP_N);

    std::ofstream out(path);
    if (!out.is_open())
    {
        return false;
    }

    out << "{\n";
    out << JSON_INDENT << "\"aggregate\": {\n";
    out << JSON_INDENT << JSON_INDENT
        << "\"cpu_pct\": " << formatFloat(cpuPct) << ",\n";
    out << JSON_INDENT << JSON_INDENT
        << "\"mem_pct\": " << formatFloat(memPct) << ",\n";
    out << JSON_INDENT << JSON_INDENT
        << "\"gpu_pct\": " << formatFloat(gpuPct) << ",\n";
    out << JSON_INDENT << JSON_INDENT
        << "\"uptime_seconds\": " << uptimeSeconds << ",\n";
    out << JSON_INDENT << JSON_INDENT
        << "\"disk_used_pct\": "
        << formatFloat(diskUsedPercent(disk.totalKb, disk.freeKb)) << ",\n";
    out << JSON_INDENT << JSON_INDENT
        << "\"net_rx_kb_s\": " << net.rxKbPerSec << ",\n";
    out << JSON_INDENT << JSON_INDENT
        << "\"net_tx_kb_s\": " << net.txKbPerSec << "\n";
    out << JSON_INDENT << "},\n";

    out << JSON_INDENT << "\"processes\": [";
    if (procs.empty())
    {
        out << "]\n";
    }
    else
    {
        out << "\n";
        for (std::size_t i = 0; i < procs.size(); ++i)
        {
            const ProcessInfo &p = procs[i];
            out << JSON_INDENT << JSON_INDENT << "{\n";
            out << JSON_INDENT << JSON_INDENT << JSON_INDENT
                << "\"pid\": " << p.pid << ",\n";
            out << JSON_INDENT << JSON_INDENT << JSON_INDENT
                << "\"parent_pid\": " << p.parentPid << ",\n";
            out << JSON_INDENT << JSON_INDENT << JSON_INDENT
                << "\"name\": " << jsonEscape(p.name) << ",\n";
            out << JSON_INDENT << JSON_INDENT << JSON_INDENT
                << "\"cpu_pct\": " << formatFloat(p.cpuPct) << ",\n";
            out << JSON_INDENT << JSON_INDENT << JSON_INDENT
                << "\"mem_pct\": " << formatFloat(p.memPct) << ",\n";
            out << JSON_INDENT << JSON_INDENT << JSON_INDENT
                << "\"read_kb_s\": " << p.diskReadKbPerSec << ",\n";
            out << JSON_INDENT << JSON_INDENT << JSON_INDENT
                << "\"write_kb_s\": " << p.diskWriteKbPerSec << "\n";
            out << JSON_INDENT << JSON_INDENT << "}";
            if (i + 1 < procs.size())
            {
                out << ",";
            }
            out << "\n";
        }
        out << JSON_INDENT << "]\n";
    }
    out << "}\n";

    out.flush();
    return static_cast<bool>(out);
}
