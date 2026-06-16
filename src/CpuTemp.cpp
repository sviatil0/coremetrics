// Author: Sviatoslav Oleksiienko <soleksiienko1@gmail.com>
//
// Per-platform CPU package temperature reader. See include/CpuTemp.hpp for
// the contract. -1.0f means "unavailable" on every backend; callers must
// not display negative temperatures.

#include "CpuTemp.hpp"

#if defined(__APPLE__)

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <cstdint>
#include <cstring>

// AppleSMC user-client selector + key for the CPU proximity sensor. The
// SMC accepts a 4-char key encoded as a big-endian uint32 plus a single
// data-type tag. TC0P returns a 2-byte "sp78" fixed-point value: integer
// degrees Celsius in the high byte, the fractional part in the low byte.
static constexpr uint32_t MS_SMC_CMD_READ_KEY = 5;
static constexpr uint32_t MS_SMC_KEY_TC0P = 0x54433050; // 'T','C','0','P'

// Mirrors the layout libsmc / smckit use for the AppleSMC user-client
// struct method. Only the fields touched here are commented; the rest
// must be zero-initialized or the call returns kIOReturnBadArgument.
struct MsSmcKeyData
{
    uint32_t key;
    uint8_t  pad0[22];
    uint32_t dataSize;
    uint32_t dataType;
    uint8_t  pad1[6];
    uint8_t  cmd;
    uint32_t pad2;
    uint8_t  bytes[32];
};

float CpuTemp::readCelsius()
{
    io_iterator_t iter = 0;
    CFMutableDictionaryRef match = IOServiceMatching("AppleSMC");
    if (match == nullptr)
    {
        return -1.0f;
    }
    // IOServiceGetMatchingServices consumes the match dictionary, so we
    // do not release it ourselves on success or failure of that call.
    if (IOServiceGetMatchingServices(MACH_PORT_NULL, match, &iter) != KERN_SUCCESS)
    {
        return -1.0f;
    }
    io_service_t service = IOIteratorNext(iter);
    IOObjectRelease(iter);
    if (service == 0)
    {
        return -1.0f;
    }

    io_connect_t conn = 0;
    kern_return_t openResult = IOServiceOpen(service, mach_task_self(), 0, &conn);
    IOObjectRelease(service);
    if (openResult != KERN_SUCCESS || conn == 0)
    {
        return -1.0f;
    }

    MsSmcKeyData input;
    MsSmcKeyData output;
    std::memset(&input, 0, sizeof(input));
    std::memset(&output, 0, sizeof(output));
    input.key = MS_SMC_KEY_TC0P;
    input.cmd = static_cast<uint8_t>(MS_SMC_CMD_READ_KEY);

    size_t outSize = sizeof(output);
    kern_return_t callResult = IOConnectCallStructMethod(
        conn, 2, &input, sizeof(input), &output, &outSize);
    IOServiceClose(conn);
    if (callResult != KERN_SUCCESS)
    {
        return -1.0f;
    }
    if (output.dataSize < 2)
    {
        return -1.0f;
    }
    // sp78: signed 8.8 fixed-point. High byte is the integer Celsius value;
    // low byte is the fractional component in 1/256ths of a degree.
    int16_t whole = static_cast<int8_t>(output.bytes[0]);
    uint8_t frac = output.bytes[1];
    float celsius = static_cast<float>(whole) + (static_cast<float>(frac) / 256.0f);
    if (celsius <= 0.0f || celsius > 150.0f)
    {
        return -1.0f;
    }
    return celsius;
}

#elif defined(__linux__)

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <string>

static bool cpuTempZoneTypeMatches(const char *type)
{
    if (type == nullptr)
    {
        return false;
    }
    if (std::strstr(type, "x86_pkg_temp") != nullptr)
    {
        return true;
    }
    if (std::strstr(type, "cpu-thermal") != nullptr)
    {
        return true;
    }
    return false;
}

float CpuTemp::readCelsius()
{
    DIR *dir = opendir("/sys/class/thermal");
    if (dir == nullptr)
    {
        return -1.0f;
    }
    float result = -1.0f;
    struct dirent *entry = nullptr;
    while ((entry = readdir(dir)) != nullptr)
    {
        // thermal_zone* directories only; skip ., .., cooling_device*, etc.
        if (std::strncmp(entry->d_name, "thermal_zone", 12) != 0)
        {
            continue;
        }
        std::string base = std::string("/sys/class/thermal/") + entry->d_name;
        std::string typePath = base + "/type";
        std::string tempPath = base + "/temp";

        FILE *typeFile = std::fopen(typePath.c_str(), "r");
        if (typeFile == nullptr)
        {
            continue;
        }
        char typeBuf[128];
        std::memset(typeBuf, 0, sizeof(typeBuf));
        size_t typeRead = std::fread(typeBuf, 1, sizeof(typeBuf) - 1, typeFile);
        std::fclose(typeFile);
        if (typeRead == 0)
        {
            continue;
        }
        // Strip trailing newline that sysfs always appends.
        if (typeBuf[typeRead - 1] == '\n')
        {
            typeBuf[typeRead - 1] = '\0';
        }
        if (!cpuTempZoneTypeMatches(typeBuf))
        {
            continue;
        }

        FILE *tempFile = std::fopen(tempPath.c_str(), "r");
        if (tempFile == nullptr)
        {
            continue;
        }
        long milliC = 0;
        int matched = std::fscanf(tempFile, "%ld", &milliC);
        std::fclose(tempFile);
        if (matched != 1)
        {
            continue;
        }
        float celsius = static_cast<float>(milliC) / 1000.0f;
        if (celsius <= 0.0f || celsius > 150.0f)
        {
            continue;
        }
        result = celsius;
        break;
    }
    closedir(dir);
    return result;
}

#elif defined(_WIN32)

// Windows backend intentionally stubbed. A proper implementation would
// query MSAcpi_ThermalZoneTemperature via WMI (CoInitializeEx +
// IWbemLocator::ConnectServer + ExecQuery on root\WMI) and translate the
// returned tenths-of-Kelvin value to Celsius. That requires linking
// wbemuuid.lib and adding COM init/teardown, which is outside the hard
// scope of this change.
float CpuTemp::readCelsius()
{
    return -1.0f;
}

#else

float CpuTemp::readCelsius()
{
    return -1.0f;
}

#endif
