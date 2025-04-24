#pragma once

#include <tavros/core/string.hpp>

namespace tavros::math
{
    class euler3;
    class mat3;
    class mat4;
    class quat;
    class vec2;
    class vec3;
    class vec4;
} // namespace tavros::math

namespace tavros::core
{

    string make_string(const math::euler3& e, int precision = 3);
    string make_string(const math::mat3& m, int precision = 3);
    string make_string(const math::mat4& m, int precision = 3);
    string make_string(const math::quat& q, int precision = 3);
    string make_string(const math::vec2& q, int precision = 3);
    string make_string(const math::vec3& q, int precision = 3);
    string make_string(const math::vec4& q, int precision = 3);

} // namespace tavros::core
