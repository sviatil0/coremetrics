#include "Battery.hpp"

#ifdef __APPLE__

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/ps/IOPSKeys.h>
#include <IOKit/ps/IOPowerSources.h>

namespace Battery
{
    // Pull an int out of a CFDictionary value if it is a CFNumber. Returns
    // the supplied default on a missing key or a type mismatch, which is
    // what the IOPS dictionary shape requires us to tolerate (some keys
    // are only published when the cell is actually in that state, e.g.
    // "Time to Full Charge" while charging).
    static int copyIntValue(CFDictionaryRef dict, CFStringRef key, int fallback)
    {
        CFTypeRef raw = CFDictionaryGetValue(dict, key);
        if (raw == nullptr)
        {
            return fallback;
        }
        if (CFGetTypeID(raw) != CFNumberGetTypeID())
        {
            return fallback;
        }
        int value = 0;
        if (!CFNumberGetValue(static_cast<CFNumberRef>(raw), kCFNumberIntType, &value))
        {
            return fallback;
        }
        return value;
    }

    static bool copyBoolValue(CFDictionaryRef dict, CFStringRef key, bool fallback)
    {
        CFTypeRef raw = CFDictionaryGetValue(dict, key);
        if (raw == nullptr)
        {
            return fallback;
        }
        if (CFGetTypeID(raw) != CFBooleanGetTypeID())
        {
            return fallback;
        }
        return CFBooleanGetValue(static_cast<CFBooleanRef>(raw)) == TRUE;
    }

    BatteryInfo read()
    {
        BatteryInfo info;

        // IOPSCopyPowerSourcesInfo returns a snapshot blob that we then
        // walk via IOPSCopyPowerSourcesList. Both calls are documented to
        // tolerate null returns on hardware that has no power sources at
        // all (typically a Mac mini / Studio / Pro), and we degrade to
        // isPresent = false in that case.
        CFTypeRef blob = IOPSCopyPowerSourcesInfo();
        if (blob == nullptr)
        {
            return info;
        }

        CFArrayRef sources = IOPSCopyPowerSourcesList(blob);
        if (sources == nullptr)
        {
            CFRelease(blob);
            return info;
        }

        CFIndex count = CFArrayGetCount(sources);
        for (CFIndex i = 0; i < count; ++i)
        {
            CFTypeRef ps = CFArrayGetValueAtIndex(sources, i);
            if (ps == nullptr)
            {
                continue;
            }
            CFDictionaryRef desc = IOPSGetPowerSourceDescription(blob, ps);
            if (desc == nullptr)
            {
                continue;
            }

            // "Is Present" is the canonical signal for an installed cell.
            // We skip entries that explicitly report not-present (some
            // docks publish a ghost UPS entry on macOS) and pick the
            // first real one. Single-battery laptops always have exactly
            // one matching entry.
            bool present = copyBoolValue(desc,
                                         CFSTR(kIOPSIsPresentKey),
                                         false);
            if (!present)
            {
                continue;
            }

            int current = copyIntValue(desc, CFSTR(kIOPSCurrentCapacityKey), 0);
            int maxCap = copyIntValue(desc, CFSTR(kIOPSMaxCapacityKey), 0);
            int percentage = 0;
            if (maxCap > 0)
            {
                // The IOPS dictionary already normalises capacity to a
                // percentage on most Macs (max = 100), but the API
                // contract is "fraction of max", so do the division and
                // clamp defensively for the edge case where current
                // momentarily exceeds max during a calibration cycle.
                long scaled = (static_cast<long>(current) * 100L)
                              / static_cast<long>(maxCap);
                if (scaled < 0)
                {
                    scaled = 0;
                }
                if (scaled > 100)
                {
                    scaled = 100;
                }
                percentage = static_cast<int>(scaled);
            }

            bool charging = copyBoolValue(desc,
                                          CFSTR(kIOPSIsChargingKey),
                                          false);

            // "Time to Empty" is only valid while discharging and is -1
            // (kIOPSTimeRemainingUnknown) when the estimate has not
            // converged. We surface "unknown" as -1 in our own struct
            // and leave the value untouched while charging.
            int timeRemaining = -1;
            if (!charging)
            {
                int raw = copyIntValue(desc,
                                       CFSTR(kIOPSTimeToEmptyKey),
                                       -1);
                if (raw >= 0)
                {
                    timeRemaining = raw;
                }
            }

            info.percentage = percentage;
            info.isCharging = charging;
            info.isPresent = true;
            info.timeRemainingMin = timeRemaining;
            break;
        }

        CFRelease(sources);
        CFRelease(blob);
        return info;
    }
}

#elif defined(__linux__)

#include <dirent.h>
#include <fstream>
#include <string>
#include <sys/types.h>

namespace Battery
{
    // Trim trailing whitespace and newlines off the values sysfs writes
    // into its single-line files. The files end in '\n' which would
    // otherwise break the std::string equality checks below.
    static std::string trim(const std::string& in)
    {
        std::string::size_type end = in.find_last_not_of(" \t\r\n");
        if (end == std::string::npos)
        {
            return std::string();
        }
        return in.substr(0, end + 1);
    }

    static std::string readLine(const std::string& path)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            return std::string();
        }
        std::string line;
        std::getline(file, line);
        return trim(line);
    }

    BatteryInfo read()
    {
        BatteryInfo info;

        // /sys/class/power_supply lists every power source the kernel
        // knows about: batteries (BAT0, BAT1), the AC adapter (AC, ACAD,
        // ADP1), and on some laptops the keyboard / mouse battery. We
        // pick the first directory whose name starts with "BAT" so the
        // common single-battery laptop case Just Works without us
        // hard-coding BAT0 (some Lenovos number from BAT1).
        DIR* dir = opendir("/sys/class/power_supply");
        if (dir == nullptr)
        {
            return info;
        }

        std::string base;
        struct dirent* entry = nullptr;
        while ((entry = readdir(dir)) != nullptr)
        {
            std::string name = entry->d_name;
            if (name.compare(0, 3, "BAT") == 0)
            {
                base = std::string("/sys/class/power_supply/") + name;
                break;
            }
        }
        closedir(dir);

        if (base.empty())
        {
            return info;
        }

        // "capacity" is an integer percentage 0..100. A missing or
        // unreadable file means the battery driver is unhealthy and we
        // degrade to "not present" rather than show 0%.
        std::string capRaw = readLine(base + "/capacity");
        if (capRaw.empty())
        {
            return info;
        }
        int percentage = 0;
        try
        {
            percentage = std::stoi(capRaw);
        }
        catch (...)
        {
            return info;
        }
        if (percentage < 0)
        {
            percentage = 0;
        }
        if (percentage > 100)
        {
            percentage = 100;
        }

        // "status" is a free-form short string. The kernel documents
        // five values: Charging, Discharging, Not charging, Full,
        // Unknown. Anything other than "Discharging" with the adapter
        // visible we treat as on-AC, since "Full" and "Not charging"
        // both imply the adapter is supplying power.
        std::string status = readLine(base + "/status");
        bool charging = (status == "Charging");

        // The kernel exposes a time-to-empty estimate only on a subset
        // of laptops, in seconds, via power_now / energy_now. Computing
        // it without the right files is unreliable, so we leave it as
        // -1 (unknown) on Linux and let the UI hide the row. The hard
        // scope of this change forbids growing the dependency surface
        // just to chase a flaky estimate.
        info.percentage = percentage;
        info.isCharging = charging;
        info.isPresent = true;
        info.timeRemainingMin = -1;
        return info;
    }
}

#elif defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace Battery
{
    BatteryInfo read()
    {
        BatteryInfo info;

        SYSTEM_POWER_STATUS status;
        if (GetSystemPowerStatus(&status) == 0)
        {
            return info;
        }

        // BatteryFlag == 128 ("No system battery") is the canonical
        // desktop / VM signal. BatteryLifePercent == 255 means
        // "unknown", which we also treat as not-present rather than
        // surface a nonsense number.
        if (status.BatteryFlag == 128 || status.BatteryLifePercent == 255)
        {
            return info;
        }

        int percentage = static_cast<int>(status.BatteryLifePercent);
        if (percentage < 0)
        {
            percentage = 0;
        }
        if (percentage > 100)
        {
            percentage = 100;
        }

        // ACLineStatus: 0 offline, 1 online, 255 unknown. We only call
        // it charging when the adapter is online AND the BatteryFlag
        // explicitly says the cell is taking charge (bit 0x08), which
        // matches what the Windows battery icon shows. Treating
        // "online but full" as charging would make the UI lie.
        bool onAc = (status.ACLineStatus == 1);
        bool charging = onAc
                        && (status.BatteryFlag != 255)
                        && ((status.BatteryFlag & 0x08) != 0);

        // BatteryLifeTime is in seconds, or -1 (0xFFFFFFFF) when
        // unknown. We only trust it while discharging: while charging
        // the field reports the time remaining on battery if the
        // adapter were unplugged right now, which is not what callers
        // want to display.
        int timeRemaining = -1;
        if (!charging && status.BatteryLifeTime != static_cast<DWORD>(-1))
        {
            timeRemaining = static_cast<int>(status.BatteryLifeTime / 60);
            if (timeRemaining < 0)
            {
                timeRemaining = -1;
            }
        }

        info.percentage = percentage;
        info.isCharging = charging;
        info.isPresent = true;
        info.timeRemainingMin = timeRemaining;
        return info;
    }
}

#else

// Unknown platform: leave the struct at its safe defaults so the
// binary still links and the UI hides the row.
namespace Battery
{
    BatteryInfo read()
    {
        return BatteryInfo();
    }
}

#endif
