#pragma once

#include <tavros/core/math.hpp>
#include <tavros/core/geometry.hpp>
#include <tavros/core/string_view.hpp>
#include <tavros/core/string.hpp>
#include <tavros/renderer/rhi/handle.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>

namespace tavros::ui
{

    namespace rhi = renderer::rhi;

    using vec2 = math::vec2;
    using size2 = math::size2;
    using isize2 = math::isize2;
    using vec3 = math::vec3;
    using vec4 = math::vec4;

    using rect2 = geometry::aabb2;

} // namespace tavros::ui
