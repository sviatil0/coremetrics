#ifndef __ABOUTTAB_HPP__
#define __ABOUTTAB_HPP__

#include <cstddef>

#include "screen.hpp"

// Paints the dynamic Host + Runtime rows of the About tab. The static
// chrome (app name, version, subtitle, footprint claim, license, source
// URL) lives in base.xml so it ships with the layout block; everything
// that depends on a live read (CPU model, OS string, hostname, total
// RAM, uptime, logical cores, battery, CPU package temp) is painted
// here so each frame reflects the current host state.
//
// Inputs are passed explicitly so the helper stays free of
// coremetrics.cpp globals. Internally calls HwInfo::cpuModel(),
// HwInfo::osVersion(), HwInfo::hostname(), HwInfo::totalRamBytes(),
// Battery::read(), and CpuTemp::readCelsius() directly; those getters
// are cheap and cached internally where it matters.
namespace AboutTab
{
    void render(Screen &dest,
                unsigned long long uptimeSeconds,
                std::size_t logicalCores);
}

#endif
