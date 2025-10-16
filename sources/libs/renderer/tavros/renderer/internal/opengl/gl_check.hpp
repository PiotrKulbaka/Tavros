#pragma once

#include <tavros/core/string_view.hpp>
#include <tavros/core/defines.hpp>

namespace tavros::renderer::rhi
{
    void gl_check_impl(const char* func, const char* file, int line);
}

#if TAV_DEBUG
    #define GL_CHECK() tavros::renderer::rhi::gl_check_impl(__FUNCTION__, __FILE__, __LINE__)
    #define GL_CALL(x)                                                    \
        do {                                                              \
            x;                                                            \
            tavros::renderer::rhi::gl_check_impl(#x, __FILE__, __LINE__); \
        } while (false)
#else
    #define GL_CHECK() ((void) 0)
    #define GL_CALL(x) (x)
#endif