#include <tavros/core/math/euler3.hpp>
#include <tavros/core/math/mat3.hpp>
#include <tavros/core/math/mat4.hpp>
#include <tavros/core/math/quat.hpp>
#include <tavros/core/math/vec2.hpp>
#include <tavros/core/math/vec3.hpp>
#include <tavros/core/math/vec4.hpp>
#include <tavros/core/math/ivec2.hpp>

#include <tavros/core/math/functions/almost_equal.hpp>
#include <tavros/core/math/functions/angle.hpp>
#include <tavros/core/math/functions/basic_math.hpp>
#include <tavros/core/math/functions/clamp.hpp>
#include <tavros/core/math/functions/conjugate.hpp>
#include <tavros/core/math/functions/cross.hpp>
#include <tavros/core/math/functions/determinant.hpp>
#include <tavros/core/math/functions/dot.hpp>
#include <tavros/core/math/functions/inverse.hpp>
#include <tavros/core/math/functions/length.hpp>
#include <tavros/core/math/functions/lerp.hpp>
#include <tavros/core/math/functions/make_euler.hpp>
#include <tavros/core/math/functions/make_mat.hpp>
#include <tavros/core/math/functions/make_quat.hpp>
// #include <tavros/core/math/functions/max.hpp>
// #include <tavros/core/math/functions/min.hpp>
#include <tavros/core/math/functions/normalize.hpp>
#include <tavros/core/math/functions/orthogonal.hpp>
#include <tavros/core/math/functions/rotate.hpp>
#include <tavros/core/math/functions/slerp.hpp>
#include <tavros/core/math/functions/transpose.hpp>
