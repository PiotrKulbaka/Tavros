#include <tavros/system/time.hpp>

#include <windows.h>

namespace
{

    static double s_qpc_to_micro = 0.0;
    static int64  s_qpc_offset_us = 0;

    void init_time_system()
    {
        static bool initialized = false;
        if (initialized) {
            return;
        }
        initialized = true;

        LARGE_INTEGER freq;
        ::QueryPerformanceFrequency(&freq);
        s_qpc_to_micro = 1'000'000.0 / static_cast<double>(freq.QuadPart);

        // Align QPC and GetTickCount (used in GetMessageTime)
        LARGE_INTEGER counter;
        ::QueryPerformanceCounter(&counter);
        DWORD tick_ms = ::GetTickCount();

        uint64 qpc_us = static_cast<uint64>(counter.QuadPart * s_qpc_to_micro);
        uint64 tick_us = static_cast<uint64>(tick_ms) * 1000ULL;
        s_qpc_offset_us = static_cast<int64>(tick_us) - static_cast<int64_t>(qpc_us);
    }
} // namespace

namespace tavros::system
{

    uint64 get_high_precision_system_time_us()
    {
        init_time_system();

        LARGE_INTEGER counter;
        ::QueryPerformanceCounter(&counter);
        return static_cast<uint64>(counter.QuadPart * s_qpc_to_micro) + s_qpc_offset_us;
    }

    uint64 get_event_time_us()
    {
        init_time_system();

        DWORD event_ms = ::GetMessageTime();
        return static_cast<uint64>(event_ms) * 1000ull;
    }

} // namespace tavros::system
