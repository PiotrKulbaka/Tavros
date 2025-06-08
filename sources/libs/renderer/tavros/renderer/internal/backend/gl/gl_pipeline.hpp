#pragma once

#include <tavros/renderer/rhi/pipeline_desc.hpp>

#include <glad/glad.h>

namespace tavros::renderer
{

    struct gl_pipeline
    {
    public:
        pipeline_desc desc;
        GLuint        program = 0;
    };

} // namespace tavros::renderer
