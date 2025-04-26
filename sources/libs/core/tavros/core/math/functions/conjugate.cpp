#include <tavros/core/math/functions/conjugate.hpp>

#include <tavros/core/math/quat.hpp>

namespace tavros::math
{

    quat conjugate(const quat& q) noexcept
    {
        return quat(-q.x, -q.y, -q.z, q.w);
    }

} // namespace tavros::math
