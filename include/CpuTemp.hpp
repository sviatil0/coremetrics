#ifndef __CPU_TEMP_HPP__
#define __CPU_TEMP_HPP__

// Author: Sviatoslav Oleksiienko <soleksiienko1@gmail.com>
//
// Per-platform CPU package temperature reader. Returns the current package
// temperature in degrees Celsius, or -1.0f when the platform cannot supply
// a value (sensor missing, query failed, or backend not implemented).
//
// Backends:
//   macOS:   IOKit AppleSMC TC0P key
//   linux:   /sys/class/thermal/thermal_zone* whose `type` contains
//            "x86_pkg_temp" or "cpu-thermal"
//   windows: stub; always returns -1.0f
//
// Callers should treat any value below 0.0f as "unavailable" and skip
// rendering rather than display a negative temperature.
namespace CpuTemp
{
    float readCelsius();
}

#endif
