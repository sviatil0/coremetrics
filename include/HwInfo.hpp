#ifndef __HW_INFO_HPP__
#define __HW_INFO_HPP__

#include <string>

// Host-level metadata snapshot. Every getter returns once-per-process info
// (CPU model never changes mid-run; the kernel version string only changes
// after a reboot; hostname only changes if the user renames the machine and
// we are happy to be stale until the next launch). All four are safe to call
// any number of times; the platform queries are cheap but the result is the
// same, so callers typically cache the four strings on startup.
//
// On any platform query failure the string getters fall back to a stable
// placeholder ("Unknown CPU", "unknown") so callers never need a try/catch
// path. totalRamBytes() returns 0 when the OS call fails (the UI can then
// hide the row).
namespace HwInfo
{
    // CPU brand string as reported by the platform. Examples:
    //   macOS  -> "Apple M3 Pro"
    //   linux  -> "Intel(R) Xeon(R) CPU @ 2.30GHz"
    //   win    -> "11th Gen Intel(R) Core(TM) i7-1185G7 @ 3.00GHz"
    // Returns "Unknown CPU" if the platform call fails.
    std::string cpuModel();

    // Short, human-readable OS string with the architecture appended in
    // parentheses where the platform exposes it. Examples:
    //   macOS  -> "macOS 14.5 (arm64)"
    //   linux  -> "Linux 5.15.0-91-generic"
    //   win    -> "Windows 11 22H2"
    // Returns "unknown" if the platform call fails.
    std::string osVersion();

    // Machine hostname as returned by gethostname / GetComputerNameA.
    // Returns "unknown" if the platform call fails or returns an empty
    // string. Never includes a trailing dot.
    std::string hostname();

    // Total physical RAM in bytes. Returns 0 if the platform call fails.
    unsigned long long totalRamBytes();
}

#endif
