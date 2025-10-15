#include <tavros/renderer/internal/opengl/gl_check.hpp>

#include <tavros/core/logger/logger.hpp>

#include <glad/glad.h>

namespace tavros::renderer::rhi
{

    void gl_check_impl(const char* func, const char* file, int line)
    {
        auto error = glGetError();
		int count = 0;
        while (error != GL_NO_ERROR && count < 100)
        {
            const char* error_str = nullptr;
            switch (error)
            {
                case GL_INVALID_ENUM:                  error_str = "GL_INVALID_ENUM"; break;
                case GL_INVALID_VALUE:                 error_str = "GL_INVALID_VALUE"; break;
                case GL_INVALID_OPERATION:             error_str = "GL_INVALID_OPERATION"; break;
                case GL_STACK_OVERFLOW:                error_str = "GL_STACK_OVERFLOW"; break;
                case GL_STACK_UNDERFLOW:               error_str = "GL_STACK_UNDERFLOW"; break;
                case GL_OUT_OF_MEMORY:                 error_str = "GL_OUT_OF_MEMORY"; break;
                case GL_INVALID_FRAMEBUFFER_OPERATION: error_str = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
                default:                               error_str = "UNKNOWN_ERROR"; break;
            }

            core::logger::print(core::severity_level::error, "gl_check", "{} ({}:{}): {}", func, file, line, error_str);
            error = glGetError();
            count++;
        }
    }

}
