#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/ids/handle_base.hpp>

namespace tavros::renderer
{
    // clang-format off
    struct texture_tag : tavros::core::handle_type_registration<0x2001> {};
    struct render_target_tag : tavros::core::handle_type_registration<0x2002> {};
    struct material_tag : tavros::core::handle_type_registration<0x2003> {};
    // clang-format on
} // namespace tavros::renderer
