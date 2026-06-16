#include "HwInfo.hpp"

#include <cstring>
#include <string>

#ifdef __APPLE__

#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>

namespace HwInfo
{
    std::string cpuModel()
    {
        // sysctlbyname with a null buffer returns the required size first,
        // including the trailing NUL. A failure (e.g. removed key on a
        // future SDK) leaves us with the safe placeholder.
        size_t len = 0;
        if (sysctlbyname("machdep.cpu.brand_string", nullptr, &len, nullptr, 0) != 0
            || len == 0)
        {
            return "Unknown CPU";
        }
        std::string buf(len, '\0');
        if (sysctlbyname("machdep.cpu.brand_string", &buf[0], &len, nullptr, 0) != 0)
        {
            return "Unknown CPU";
        }
        // Strip the trailing NUL byte that sysctl includes in len.
        if (!buf.empty() && buf.back() == '\0')
        {
            buf.pop_back();
        }
        if (buf.empty())
        {
            return "Unknown CPU";
        }
        return buf;
    }

    std::string osVersion()
    {
        // We want "macOS <product>.<version> (<arch>)". `uname` gives us
        // the Darwin kernel version, which is not what users recognize, so
        // we ask sysctl for kern.osproductversion (the marketing version,
        // e.g. 14.5) and hw.machine (e.g. arm64 / x86_64).
        std::string product = "unknown";
        size_t len = 0;
        if (sysctlbyname("kern.osproductversion", nullptr, &len, nullptr, 0) == 0
            && len > 0)
        {
            std::string buf(len, '\0');
            if (sysctlbyname("kern.osproductversion", &buf[0], &len, nullptr, 0) == 0)
            {
                if (!buf.empty() && buf.back() == '\0')
                {
                    buf.pop_back();
                }
                if (!buf.empty())
                {
                    product = buf;
                }
            }
        }

        std::string arch;
        size_t archLen = 0;
        if (sysctlbyname("hw.machine", nullptr, &archLen, nullptr, 0) == 0 && archLen > 0)
        {
            std::string buf(archLen, '\0');
            if (sysctlbyname("hw.machine", &buf[0], &archLen, nullptr, 0) == 0)
            {
                if (!buf.empty() && buf.back() == '\0')
                {
                    buf.pop_back();
                }
                arch = buf;
            }
        }

        std::string out = "macOS ";
        out += product;
        if (!arch.empty())
        {
            out += " (";
            out += arch;
            out += ")";
        }
        return out;
    }

    std::string hostname()
    {
        // HOST_NAME_MAX is not always defined on macOS SDKs; 256 is the
        // safe upper bound used elsewhere in the repo's platform code.
        char buf[256];
        if (gethostname(buf, sizeof(buf)) != 0)
        {
            return "unknown";
        }
        // gethostname is not guaranteed to NUL-terminate on truncation.
        buf[sizeof(buf) - 1] = '\0';
        std::string out(buf);
        if (out.empty())
        {
            return "unknown";
        }
        return out;
    }

    unsigned long long totalRamBytes()
    {
        unsigned long long mem = 0;
        size_t len = sizeof(mem);
        if (sysctlbyname("hw.memsize", &mem, &len, nullptr, 0) != 0)
        {
            return 0;
        }
        return mem;
    }
}

#elif defined(__linux__)

#include <fstream>
#include <sstream>
#include <sys/utsname.h>
#include <unistd.h>

namespace HwInfo
{
    std::string cpuModel()
    {
        // /proc/cpuinfo lists every logical CPU; the "model name" field is
        // identical across cores on x86 and ARM, so the first match is
        // enough. On some ARM kernels the field is missing entirely and
        // we fall back to the safe placeholder.
        std::ifstream file("/proc/cpuinfo");
        if (!file.is_open())
        {
            return "Unknown CPU";
        }
        std::string line;
        while (std::getline(file, line))
        {
            const std::string key = "model name";
            if (line.compare(0, key.size(), key) == 0)
            {
                std::string::size_type colon = line.find(':');
                if (colon == std::string::npos)
                {
                    continue;
                }
                std::string value = line.substr(colon + 1);
                // Trim leading whitespace; the value is typically " Intel ..."
                std::string::size_type start = value.find_first_not_of(" \t");
                if (start == std::string::npos)
                {
                    continue;
                }
                std::string::size_type end = value.find_last_not_of(" \t\r\n");
                std::string trimmed = value.substr(start, end - start + 1);
                if (!trimmed.empty())
                {
                    return trimmed;
                }
            }
        }
        return "Unknown CPU";
    }

    std::string osVersion()
    {
        struct utsname u;
        if (uname(&u) != 0)
        {
            return "unknown";
        }
        std::string out = u.sysname;
        if (out.empty())
        {
            return "unknown";
        }
        if (u.release[0] != '\0')
        {
            out += " ";
            out += u.release;
        }
        return out;
    }

    std::string hostname()
    {
        char buf[256];
        if (gethostname(buf, sizeof(buf)) != 0)
        {
            return "unknown";
        }
        buf[sizeof(buf) - 1] = '\0';
        std::string out(buf);
        if (out.empty())
        {
            return "unknown";
        }
        return out;
    }

    unsigned long long totalRamBytes()
    {
        // /proc/meminfo "MemTotal:" is in kilobytes, suffixed with " kB".
        std::ifstream file("/proc/meminfo");
        if (!file.is_open())
        {
            return 0;
        }
        std::string line;
        while (std::getline(file, line))
        {
            const std::string key = "MemTotal:";
            if (line.compare(0, key.size(), key) == 0)
            {
                std::istringstream iss(line.substr(key.size()));
                unsigned long long kb = 0;
                iss >> kb;
                if (kb == 0)
                {
                    return 0;
                }
                return kb * 1024ULL;
            }
        }
        return 0;
    }
}

#elif defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#include <windows.h>
#include <sysinfoapi.h>

// RtlGetVersion lives in ntdll.dll and is the only documented way to
// avoid the GetVersionEx compatibility-shim that lies about the OS
// version on Windows 8.1+ unless the binary embeds a compatibility
// manifest. Declared locally to avoid pulling in the full DDK header.
typedef LONG (WINAPI *RtlGetVersionFn)(PRTL_OSVERSIONINFOW);

namespace HwInfo
{
    std::string cpuModel()
    {
        // The CPU brand string lives in the registry under
        // HKLM\HARDWARE\DESCRIPTION\System\CentralProcessor\0 as the
        // ProcessorNameString REG_SZ value. RegGetValueA gives us the
        // ANSI form directly so we do not have to deal with UTF-16.
        char buf[256];
        DWORD size = sizeof(buf);
        LONG rc = RegGetValueA(HKEY_LOCAL_MACHINE,
                               "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                               "ProcessorNameString",
                               RRF_RT_REG_SZ,
                               nullptr,
                               buf,
                               &size);
        if (rc != ERROR_SUCCESS)
        {
            return "Unknown CPU";
        }
        // RegGetValueA NUL-terminates within the reported size.
        std::string out(buf);
        // Trim any trailing whitespace the registry value often carries.
        std::string::size_type end = out.find_last_not_of(" \t\r\n");
        if (end == std::string::npos)
        {
            return "Unknown CPU";
        }
        out.resize(end + 1);
        if (out.empty())
        {
            return "Unknown CPU";
        }
        return out;
    }

    std::string osVersion()
    {
        // Prefer RtlGetVersion to dodge the GetVersionEx manifest lie.
        // Fall back to GetVersionEx when ntdll is unavailable for any
        // reason (it never is, on a stock system, but we want a clean
        // failure path).
        RTL_OSVERSIONINFOW info;
        std::memset(&info, 0, sizeof(info));
        info.dwOSVersionInfoSize = sizeof(info);

        HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
        bool gotVersion = false;
        if (ntdll != nullptr)
        {
            RtlGetVersionFn fn = reinterpret_cast<RtlGetVersionFn>(
                GetProcAddress(ntdll, "RtlGetVersion"));
            if (fn != nullptr && fn(&info) == 0)
            {
                gotVersion = true;
            }
        }
        if (!gotVersion)
        {
            return "unknown";
        }

        std::string product = "Windows";
        // Windows 11 reports major == 10 with build >= 22000, which is the
        // documented way Microsoft distinguishes 10 from 11.
        if (info.dwMajorVersion == 10 && info.dwBuildNumber >= 22000)
        {
            product += " 11";
        }
        else if (info.dwMajorVersion == 10)
        {
            product += " 10";
        }
        else
        {
            product += " ";
            product += std::to_string(info.dwMajorVersion);
            product += ".";
            product += std::to_string(info.dwMinorVersion);
        }

        // Append the build as a short tag so support tickets can match it.
        product += " build ";
        product += std::to_string(info.dwBuildNumber);
        return product;
    }

    std::string hostname()
    {
        char buf[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD size = sizeof(buf);
        if (GetComputerNameA(buf, &size) == 0)
        {
            return "unknown";
        }
        std::string out(buf, size);
        if (out.empty())
        {
            return "unknown";
        }
        return out;
    }

    unsigned long long totalRamBytes()
    {
        MEMORYSTATUSEX status;
        std::memset(&status, 0, sizeof(status));
        status.dwLength = sizeof(status);
        if (GlobalMemoryStatusEx(&status) == 0)
        {
            return 0;
        }
        return static_cast<unsigned long long>(status.ullTotalPhys);
    }
}

#else

// Unknown platform: provide the safe defaults so the binary still links.
namespace HwInfo
{
    std::string cpuModel()
    {
        return "Unknown CPU";
    }

    std::string osVersion()
    {
        return "unknown";
    }

    std::string hostname()
    {
        return "unknown";
    }

    unsigned long long totalRamBytes()
    {
        return 0;
    }
}

#endif
