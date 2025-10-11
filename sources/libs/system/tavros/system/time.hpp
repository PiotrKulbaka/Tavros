#pragma once

#include <tavros/core/types.hpp>

namespace tavros::system
{

    /**
     * @brief Returns the current high-precision system time in microseconds.
     *
     * Retrieves a monotonic, high-resolution timestamp representing the time elapsed
     * since the system was started. The returned value is based on the system's
     * performance counter (e.g., QueryPerformanceCounter on Windows) and is suitable
     * for precise timing, profiling, and synchronization tasks.
     *
     * @return Current system time in microseconds since system startup.
     */
    uint64 get_high_precision_system_time_us();

} // namespace tavros::system
