#ifndef __BATTERY_HPP__
#define __BATTERY_HPP__

// Snapshot of the host's primary battery, intended for the UI's status
// strip. Desktops and other machines with no battery report
// isPresent = false; the UI hides the row in that case so the caller
// never needs to differentiate "no battery" from "battery query
// failed" via a second channel. Any platform query failure also
// degrades to isPresent = false with the other fields left at their
// safe defaults below so callers never need a try/catch path.
//
// The struct is intentionally tiny and trivially copyable: the
// metrics thread polls Battery::read() on its existing cadence and
// the result is handed across to the render thread by value.
namespace Battery
{
    struct BatteryInfo
    {
        // 0..100. Meaningful only when isPresent == true. Defaults to 0
        // so a stale or failed read renders as an empty bar rather
        // than a "full" one, which would be misleading.
        int percentage = 0;

        // True while the AC adapter is supplying power and the cell
        // is taking charge. On a desktop or on a laptop with the
        // adapter removed and the battery discharging this is false.
        bool isCharging = false;

        // False on desktops, in VMs without a virtual battery, and
        // on any platform call failure. The UI uses this as the
        // single "should we draw the battery row?" signal.
        bool isPresent = false;

        // Estimated minutes of runtime remaining on the current
        // charge when discharging. -1 means "unknown" (the platform
        // does not expose it, or the estimate has not converged
        // yet, which is common in the first minute after unplug).
        // While charging the value is undefined and callers should
        // ignore it; isCharging is the gate.
        int timeRemainingMin = -1;
    };

    // Cheap to call: each platform hits a single OS API and the
    // result is returned by value. Safe to call from any thread;
    // there is no shared state. The caller is expected to throttle
    // (the metrics thread already runs on a fixed tick).
    BatteryInfo read();
}

#endif
