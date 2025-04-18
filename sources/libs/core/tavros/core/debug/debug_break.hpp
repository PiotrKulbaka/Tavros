#pragma once

#include <tavros/core/defines.hpp>

#if TAV_DEBUG
void tav_debug_break_impl();
    #define TAV_DEBUG_BREAK() tav_debug_break_impl()
#else
    #define TAV_DEBUG_BREAK() ((void) 0)
#endif
