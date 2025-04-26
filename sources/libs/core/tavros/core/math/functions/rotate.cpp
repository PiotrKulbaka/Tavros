#include <tavros/core/math/functions/rotate.hpp>

#include <tavros/core/math/functions/inverse.hpp>
#include <tavros/core/math/functions/cross.hpp>
#include <tavros/core/math/quat.hpp>
#include <tavros/core/math/vec3.hpp>

namespace tavros::math
{

    vec3 rotate_point(const quat& q, const vec3& p) noexcept
    {
        vec3 qvec(q.x, q.y, q.z);
        vec3 uv = cross(p, qvec);
        vec3 uuv = cross(uv, qvec);
        return p + ((uv * q.w) + uuv) * 2.0f;
        //
        //        float xx = q.x * q.x;
        //        float yy = q.y * q.y;
        //        float zz = q.z * q.z;
        //        float ww = q.w * q.w;
        //
        //        float xxzz = xx - zz;
        //        float wwyy = ww - yy;
        //
        //        float xw2 = q.x * q.w * 2.0f;
        //        float xy2 = q.x * q.y * 2.0f;
        //        float xz2 = q.x * q.z * 2.0f;
        //        float yw2 = q.y * q.w * 2.0f;
        //        float yz2 = q.y * q.z * 2.0f;
        //        float zw2 = q.z * q.w * 2.0f;
        //
        //        vec3 vb = vec3(
        //            (xxzz + wwyy) * p.x		+ (xy2 + zw2) * p.y		+ (xz2 - yw2) * p.z,
        //            (xy2 - zw2) * p.x			+ (yy + ww - xx - zz) * p.y	+ (yz2 + xw2) * p.z,
        //            (xz2 + yw2) * p.x			+ (yz2 - xw2) * p.y		+ (wwyy - xxzz) * p.z
        //        );
        //
        //        return vb;
    }

} // namespace tavros::math
