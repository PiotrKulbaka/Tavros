#include <tavros/core/math/utils/make_string.hpp>

#include <tavros/core/math/euler3.hpp>
#include <tavros/core/math/mat3.hpp>
#include <tavros/core/math/mat4.hpp>
#include <tavros/core/math/quat.hpp>
#include <tavros/core/math/vec2.hpp>
#include <tavros/core/math/vec3.hpp>
#include <tavros/core/math/vec4.hpp>

#include <cstdlib>

namespace tavros::core
{

    string make_string(const math::euler3& e, int precision)
    {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "[%.*f, %.*f, %.*f]", precision, e.roll, precision, e.pitch, precision, e.yaw);
        return core::string(buffer);
    }

    string make_string(const math::mat3& m, int precision)
    {
        char buffer[256];
        // clang-format off
        snprintf(buffer, sizeof(buffer), 
            "[[%.*f, %.*f, %.*f], [%.*f, %.*f, %.*f], [%.*f, %.*f, %.*f]",
            precision, m.cols[0].x, precision, m.cols[0].y, precision, m.cols[0].z,
            precision, m.cols[1].x, precision, m.cols[1].y, precision, m.cols[1].z,
            precision, m.cols[2].x, precision, m.cols[2].y, precision, m.cols[2].z
        );
        // clang-format on
        return string(buffer);
    }

    string make_string(const math::mat4& m, int precision)
    {
        char buffer[256];
        // clang-format off
        snprintf(buffer, sizeof(buffer), 
            "[[%.*f, %.*f, %.*f, %.*f], [%.*f, %.*f, %.*f, %.*f], [%.*f, %.*f, %.*f, %.*f], [%.*f, %.*f, %.*f, %.*f]]",
            precision, m.cols[0].x, precision, m.cols[0].y, precision, m.cols[0].z, precision, m.cols[0].w,
            precision, m.cols[1].x, precision, m.cols[1].y, precision, m.cols[1].z, precision, m.cols[1].w,
            precision, m.cols[2].x, precision, m.cols[2].y, precision, m.cols[2].z, precision, m.cols[2].w,
            precision, m.cols[3].x, precision, m.cols[3].y, precision, m.cols[3].z, precision, m.cols[3].w
        );
        // clang-format on
        return string(buffer);
    }

    string make_string(const math::quat& q, int precision)
    {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "[%.*f, %.*f, %.*f, %.*f]", precision, q.x, precision, q.y, precision, q.z, precision, q.w);
        return string(buffer);
    }

    string make_string(const math::vec2& v, int precision)
    {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "[%.*f, %.*f]", precision, v.x, precision, v.y);
        return string(buffer);
    }

    string make_string(const math::vec3& v, int precision)
    {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "[%.*f, %.*f, %.*f]", precision, v.x, precision, v.y, precision, v.z);
        return string(buffer);
    }

    string make_string(const math::vec4& v, int precision)
    {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "[%.*f, %.*f, %.*f, %.*f]", precision, v.x, precision, v.y, precision, v.z, precision, v.w);
        return string(buffer);
    }

} // namespace tavros::core
