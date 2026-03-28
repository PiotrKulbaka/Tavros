#include <tavros/core/math/functions/rotate.hpp>

#include <tavros/core/math/functions/inverse.hpp>
#include <tavros/core/math/functions/cross.hpp>
#include <tavros/core/math/quat.hpp>
#include <tavros/core/math/vec3.hpp>

namespace tavros::math
{

    vec3 rotate_point(const quat& q, const vec3& p) noexcept
    {
        const vec3 qvec(q.x, q.y, q.z);
        const vec3 t = cross(qvec, p) * 2.0f;
        return p + t * q.w + cross(qvec, t);
    }

} // namespace tavros::math
