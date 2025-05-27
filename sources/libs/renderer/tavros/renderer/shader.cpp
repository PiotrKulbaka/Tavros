#include <tavros/renderer/shader.hpp>

#include <glad/glad.h>

using namespace tavros::renderer;

namespace
{
    tavros::core::logger logger("shader");

    GLuint compile_shader(tavros::core::string_view program, GLenum shader_type)
    {
        auto shader = glCreateShader(shader_type);
        if (shader == 0) {
            logger.error("glCreateShader() returns 0");
            return 0;
        }

        // compile shader
        const char* program_text = program.data();
        auto        text_length = static_cast<GLint>(program.size());
        glShaderSource(shader, 1, &program_text, &text_length);
        glCompileShader(shader);

        // check compile status
        GLint status;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (!status) {
            char buffer[4096];
            glGetShaderInfoLog(shader, sizeof(buffer), nullptr, buffer);
            logger.error("glCompileShader() failure:\n%s", buffer);
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }

    GLuint link_program(GLuint vert_shader, GLuint frag_shader)
    {
        auto failed = false;

        auto program = glCreateProgram();
        if (!program) {
            logger.error("glCreateProgram() failed");
            return 0;
        }

        glAttachShader(program, vert_shader);
        glAttachShader(program, frag_shader);
        glLinkProgram(program);

        // check link status
        GLint status;
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if (!status) {
            char buffer[4096];
            glGetProgramInfoLog(program, sizeof(buffer), nullptr, buffer);
            logger.error("glLinkProgram() failed:\n%s", buffer);
            failed = true;
        }

        // validate program
        glValidateProgram(program);
        glGetProgramiv(program, GL_VALIDATE_STATUS, &status);
        if (!status) {
            char buffer[4096];
            glGetProgramInfoLog(program, sizeof(buffer), nullptr, buffer);
            logger.error("glValidateProgram() failed:\n%s", buffer);
            failed = true;
        }

        glDetachShader(program, vert_shader);
        glDetachShader(program, frag_shader);

        if (failed) {
            glDeleteProgram(program);
            return 0;
        }

        return program;
    }
} // namespace


shader shader::create(core::string_view vert_prog, core::string_view frag_prog)
{
    auto v = compile_shader(vert_prog, GL_VERTEX_SHADER);
    auto f = compile_shader(frag_prog, GL_FRAGMENT_SHADER);
    auto p = link_program(v, f);
    glDeleteShader(v);
    glDeleteShader(f);
    return shader(p);
}

shader::shader(uint32 handle)
    : m_program(handle)
{
}

shader::~shader()
{
}

void shader::use()
{
    glUseProgram(m_program);
}

void shader::unuse()
{
    glUseProgram(0);
}

uint32 shader::handle() const noexcept
{
    return m_program;
}
