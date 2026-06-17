#include "AboutTab.hpp"

#include <cstdio>
#include <string>

#include "Battery.hpp"
#include "CpuTemp.hpp"
#include "HwInfo.hpp"
#include "ProcessUtils.hpp"
#include "Theme.hpp"
#include "font.hpp"
#include "vec2.hpp"
#include "vec3.hpp"

// Mirrors src/UptimeAndLoad.cpp in shape: pure helpers + a single
// render entry point. The static About-tab chrome (name, version,
// subtitle, license, source URL) is declared in base.xml; this TU
// fills in the rows that need a fresh read each frame.

namespace AboutTab
{
    // GB with one decimal. 36507549696 bytes -> "34.0 GB" (binary GB).
    // Matches how the rest of the app talks about RAM sizes (formatGbString
    // returns the integer GB component for the System-tab readouts; this
    // helper adds the decimal place for the About-tab one-screen summary).
    static std::string formatGB(unsigned long long bytes)
    {
        if (bytes == 0)
        {
            return "--";
        }
        double gb = static_cast<double>(bytes)
                  / (1024.0 * 1024.0 * 1024.0);
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.1f GB", gb);
        return std::string(buf);
    }

    static std::string formatBatteryString(const Battery::BatteryInfo &info)
    {
        char buf[96];
        if (info.isCharging)
        {
            std::snprintf(buf, sizeof(buf), "%d%%  charging",
                          info.percentage);
            return std::string(buf);
        }
        if (info.timeRemainingMin > 0)
        {
            int hours = info.timeRemainingMin / 60;
            int mins = info.timeRemainingMin % 60;
            std::snprintf(buf, sizeof(buf),
                          "%d%%  on battery  %dh %dm remaining",
                          info.percentage, hours, mins);
            return std::string(buf);
        }
        std::snprintf(buf, sizeof(buf), "%d%%  on battery",
                      info.percentage);
        return std::string(buf);
    }

    static std::string formatTempString(float celsius)
    {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.1f C", celsius);
        return std::string(buf);
    }

    void render(Screen &dest,
                unsigned long long uptimeSeconds,
                std::size_t logicalCores)
    {
        const vec3 sectionColor = Theme::accent();
        const vec3 headerColor = Theme::textPrimary();
        const vec3 valueColor = Theme::textDim();

        // Left column anchor and the y baseline for the Host section.
        // The static App-info block in base.xml occupies y=80..148; we
        // start the dynamic Host section at y=192 with a ~32 px gap so
        // the two read as distinct cards on the 8 px grid.
        constexpr int LEFT_X = 24;
        constexpr int VALUE_X = 168;

        constexpr int HOST_HEADER_Y = 192;
        constexpr int HOST_FIRST_ROW_Y = 216;
        constexpr int ROW_STEP = 20;

        Font::drawText(dest, "Host", ivec2(LEFT_X, HOST_HEADER_Y),
                       sectionColor);

        int y = HOST_FIRST_ROW_Y;
        Font::drawText(dest, "CPU", ivec2(LEFT_X, y), headerColor);
        Font::drawText(dest, HwInfo::cpuModel(), ivec2(VALUE_X, y),
                       valueColor);
        y += ROW_STEP;

        Font::drawText(dest, "OS", ivec2(LEFT_X, y), headerColor);
        Font::drawText(dest, HwInfo::osVersion(), ivec2(VALUE_X, y),
                       valueColor);
        y += ROW_STEP;

        Font::drawText(dest, "Hostname", ivec2(LEFT_X, y), headerColor);
        Font::drawText(dest, HwInfo::hostname(), ivec2(VALUE_X, y),
                       valueColor);
        y += ROW_STEP;

        Font::drawText(dest, "RAM", ivec2(LEFT_X, y), headerColor);
        Font::drawText(dest, formatGB(HwInfo::totalRamBytes()),
                       ivec2(VALUE_X, y), valueColor);
        y += ROW_STEP;

        // Runtime section header sits ~32 px below the last Host row
        // so the gap matches the Host-vs-App-info card separation.
        int runtimeHeaderY = y + 12;
        Font::drawText(dest, "Runtime", ivec2(LEFT_X, runtimeHeaderY),
                       sectionColor);

        y = runtimeHeaderY + 24;
        Font::drawText(dest, "Uptime", ivec2(LEFT_X, y), headerColor);
        Font::drawText(dest, formatUptimeString(uptimeSeconds),
                       ivec2(VALUE_X, y), valueColor);
        y += ROW_STEP;

        Font::drawText(dest, "Cores", ivec2(LEFT_X, y), headerColor);
        Font::drawText(dest, std::to_string(logicalCores),
                       ivec2(VALUE_X, y), valueColor);
        y += ROW_STEP;

        // Battery and temperature are optional: skip the row if the
        // platform cannot supply a value. Hidden rows do not shift the
        // license/source chrome at y=432 because the row count is
        // bounded (max 4 runtime rows) within the available band.
        Battery::BatteryInfo bat = Battery::read();
        if (bat.isPresent)
        {
            Font::drawText(dest, "Battery", ivec2(LEFT_X, y), headerColor);
            Font::drawText(dest, formatBatteryString(bat),
                           ivec2(VALUE_X, y), valueColor);
            y += ROW_STEP;
        }

        float temp = CpuTemp::readCelsius();
        if (temp >= 0.0f)
        {
            Font::drawText(dest, "CPU temp", ivec2(LEFT_X, y),
                           headerColor);
            Font::drawText(dest, formatTempString(temp),
                           ivec2(VALUE_X, y), valueColor);
        }
    }
}
