#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/string_view.hpp>

namespace tavros::renderer::rhi
{

    enum class buffer_usage : uint8;
    enum class buffer_access : uint8;
    enum class texture_type : uint8;
    enum class attribute_type : uint8;
    enum class attribute_format : uint8;

    core::string_view to_string(buffer_usage usage);

    core::string_view to_string(buffer_access access);

    core::string_view to_string(texture_type type);

    core::string_view to_string(attribute_type type);

    core::string_view to_string(attribute_format format);

} // namespace tavros::renderer::rhi
