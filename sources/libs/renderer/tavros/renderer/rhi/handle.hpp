#pragma once

#include <tavros/core/types.hpp>

namespace tavros::renderer
{

    template<typename ResourceTag>
    struct handle_base
    {
        uint32 id = 0;
    };

    struct sampler_tag
    {
    };
    struct texture2d_tag
    {
    };
    struct pipeline_tag
    {
    };

    using sampler_handle = handle_base<sampler_tag>;
    using texture2d_handle = handle_base<texture2d_tag>;
    using pipeline_handle = handle_base<pipeline_tag>;

} // namespace tavros::renderer
