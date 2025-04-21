#include <tavros/core/math/quat.hpp>

#include <tavros/core/debug/assert.hpp>

using namespace tavros::core::math;

quat::quat(const euler3& euler) noexcept
{
    *this = from_euler(euler);
}

quat::quat(const vec3& axis, float angle_rad) noexcept
{
    *this = from_axis_angle(axis, angle_rad);
}

bool quat::almost_equal(const quat& other, float epsilon) const noexcept
{
    return vec.almost_equal(other.vec, epsilon);
}

quat& quat::operator+=(const quat& other) noexcept
{
    vec += other.vec;
    return *this;
}

quat& quat::operator-=(const quat& other) noexcept
{
    vec -= other.vec;
    return *this;
}

quat& quat::operator*=(const quat& other) noexcept
{
    auto nx = w * other.x + x * other.w + y * other.z - z * other.y;
    auto ny = w * other.y - x * other.z + y * other.w + z * other.x;
    auto nz = w * other.z + x * other.y - y * other.x + z * other.w;
    auto nw = w * other.w - x * other.x - y * other.y - z * other.z;
    x = nx;
    y = ny;
    z = nz;
    w = nw;
    return *this;
}

quat& quat::operator*=(float scalar) noexcept
{
    x *= scalar;
    y *= scalar;
    z *= scalar;
    w *= scalar;
    return *this;
}

quat quat::operator-() const noexcept
{
    return quat(-x, -y, -z, -w);
}

quat quat::operator+(const quat& other) const noexcept
{
    return quat(*this) += other;
}

quat quat::operator-(const quat& other) const noexcept
{
    return quat(*this) -= other;
}

quat quat::operator*(const quat& other) const noexcept
{
    return quat(*this) *= other;
}

quat quat::operator*(float scalar) const noexcept
{
    return quat(*this) *= scalar;
}

float quat::dot(const quat& other) const noexcept
{
    return x * other.x + y * other.y + z * other.z + w * other.w;
}

bool quat::is_normalized() const noexcept
{
    return std::abs(length() - 1.0f) <= k_epsilon6;
}

float quat::length() const noexcept
{
    return std::sqrt(dot(*this));
}

quat quat::normalized() const noexcept
{
    float len = length();
    TAV_ASSERT(len > 0.0f);
    return len > 0.0f ? (*this) * (1.0f / len) : identity();
}

quat quat::conjugated() const noexcept
{
    return quat(-x, -y, -z, w);
}

quat quat::inversed() const noexcept
{
    return conjugated().normalized();
}

vec3 quat::rotate_point(const vec3& point) const noexcept
{
    quat inv = inversed();
    quat p(point.x, point.y, point.z, 0);
    auto res = *this * p * inv;
    return vec3{res.x, res.y, res.z};
}

quat quat::slerp(const quat& target, float coef) const noexcept
{
    float cos_theta = dot(target);
    quat  to = target;
    if (cos_theta < 0.0f) {
        to = -to;
        cos_theta = -cos_theta;
    }
    if (cos_theta > 0.9995f) {
        return ((*this) * (1.0f - coef) + to * coef).normalized();
    }
    auto theta = std::acos(cos_theta);
    auto sin_theta = std::sqrt(1.0f - cos_theta * cos_theta);
    auto a = std::sin((1.0f - coef) * theta) / sin_theta;
    auto b = std::sin(coef * theta) / sin_theta;
    return (*this) * a + to * b;
}

mat3 quat::to_mat3() const noexcept
{
    auto xx = x * x, yy = y * y, zz = z * z;
    auto xy = x * y, xz = x * z, yz = y * z;
    auto wx = w * x, wy = w * y, wz = w * z;
    return mat3(
        {1.0f - 2.0f * (yy + zz), 2.0f * (xy - wz), 2.0f * (xz + wy)},
        {2.0f * (xy + wz), 1.0f - 2.0f * (xx + zz), 2.0f * (yz - wx)},
        {2.0f * (xz - wy), 2.0f * (yz + wx), 1.0f - 2.0f * (xx + yy)}
    );
}

mat4 quat::to_mat4() const noexcept
{
    mat3 m = to_mat3();
    return mat4(
        {m.cols[0].x, m.cols[0].y, m.cols[0].z, 0.0f},
        {m.cols[1].x, m.cols[1].y, m.cols[1].z, 0.0f},
        {m.cols[2].x, m.cols[2].y, m.cols[2].z, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f}
    );
}

euler3 quat::to_euler() const noexcept
{
    auto sinr_cosp = 2.0f * (w * x + y * z);
    auto cosr_cosp = 1.0f - 2.0f * (x * x + y * y);
    auto pitch = std::atan2(sinr_cosp, cosr_cosp);

    auto sinp = 2.0f * (w * y - z * x);
    auto yaw = std::abs(sinp) >= 1.0f ? copysignf(k_pi_div_2, sinp) : std::asin(sinp);

    auto siny_cosp = 2.0f * (w * z + x * y);
    auto cosy_cosp = 1.0f - 2.0f * (y * y + z * z);
    auto roll = std::atan2(siny_cosp, cosy_cosp);

    return euler3(pitch, yaw, roll);
}

float quat::to_axis_angle(vec3& out_axis) const noexcept
{
    quat q = normalized();
    auto angle = 2.0f * std::acos(q.w);
    auto s = std::sqrt(1.0f - q.w * q.w);
    if (s < k_epsilon6) {
        out_axis = vec3(1.0f, 0.0f, 0.0f);
    } else {
        out_axis = vec3(q.x / s, q.y / s, q.z / s);
    }
    return angle;
}

const float* quat::data() const noexcept
{
    return &x;
}

float* quat::data() noexcept
{
    return &x;
}

tavros::core::string quat::to_string(int precision) const
{
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "[%.*f, %.*f, %.*f, %.*f]", precision, x, precision, y, precision, z, precision, w);
    return string(buffer);
}

constexpr quat quat::identity() noexcept
{
    return quat(0.0f, 0.0f, 0.0f, 1.0f);
}

quat quat::from_axis_angle(const vec3& axis, float angle_rad) noexcept
{
    auto half = angle_rad * 0.5f;
    auto s = std::sin(half);
    return quat(axis.x * s, axis.y * s, axis.z * s, std::cos(half));
}

quat quat::from_euler(const euler3& euler) noexcept
{
    auto cx = std::cos(euler.roll * 0.5f);
    auto sx = std::sin(euler.roll * 0.5f);
    auto cy = std::cos(euler.pitch * 0.5f);
    auto sy = std::sin(euler.pitch * 0.5f);
    auto cz = std::cos(euler.yaw * 0.5f);
    auto sz = std::sin(euler.yaw * 0.5f);

    return quat(
        sx * cy * cz + cx * sy * sz,
        cx * sy * cz - sx * cy * sz,
        cx * cy * sz + sx * sy * cz,
        cx * cy * cz - sx * sy * sz
    );
}

quat quat::from_to_rotation(const vec3& from, const vec3& to) noexcept
{
    auto dot_val = from.dot(to);
    if (dot_val >= 1.0f - k_epsilon6) {
        return identity();
    }
    if (dot_val <= -1.0f + k_epsilon6) {
        vec3 axis = from.orthogonal().normalized();
        return from_axis_angle(axis, k_pi);
    }
    vec3 axis = from.cross(to);
    auto s = std::sqrt((1.0f + dot_val) * 2.0f);
    auto inv_s = 1.0f / s;
    return quat(axis.x * inv_s, axis.y * inv_s, axis.z * inv_s, s * 0.5f).normalized();
}

quat quat::look_rotation(const vec3& forward, const vec3& up) noexcept
{
    vec3 f = forward.normalized();
    vec3 r = up.cross(f).normalized();
    vec3 u = f.cross(r);
    mat3 m(r, u, f);
    auto trace = m[0][0] + m[1][1] + m[2][2];

    if (trace > 0.0f) {
        auto s = std::sqrt(trace + 1.0f) * 2.0f;
        return quat(
            (m[2][1] - m[1][2]) / s,
            (m[0][2] - m[2][0]) / s,
            (m[1][0] - m[0][1]) / s,
            0.25f * s
        );
    } else {
        if (m[0][0] > m[1][1] && m[0][0] > m[2][2]) {
            auto s = std::sqrt(1.0f + m[0][0] - m[1][1] - m[2][2]) * 2.0f;
            return quat(
                0.25f * s,
                (m[0][1] + m[1][0]) / s,
                (m[0][2] + m[2][0]) / s,
                (m[2][1] - m[1][2]) / s
            );
        } else if (m[1][1] > m[2][2]) {
            auto s = std::sqrt(1.0f + m[1][1] - m[0][0] - m[2][2]) * 2.0f;
            return quat(
                (m[0][1] + m[1][0]) / s,
                0.25f * s,
                (m[1][2] + m[2][1]) / s,
                (m[0][2] - m[2][0]) / s
            );
        } else {
            auto s = std::sqrt(1.0f + m[2][2] - m[0][0] - m[1][1]) * 2.0f;
            return quat(
                (m[0][2] + m[2][0]) / s,
                (m[1][2] + m[2][1]) / s,
                0.25f * s,
                (m[1][0] - m[0][1]) / s
            );
        }
    }
}
