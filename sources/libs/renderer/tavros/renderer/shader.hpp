#pragma once

#include <tavros/core/prelude.hpp>

namespace tavros::renderer
{

    class shader
    {
    public:
        static shader create(core::string_view vert_prog, core::string_view frag_prog);

    public:
        shader(uint32 handle);

        ~shader();

        void use();

        void unuse();

        uint32 handle() const noexcept;

    private:
        uint32 m_program;
    };

} // namespace tavros::renderer
