#pragma once

#include <tavros/core/math.hpp>

namespace tavros::geometry
{

    /**
     * @brief Represents thickness values for the four sides of a rectangle.
     *
     * Stores left, top, right, and bottom offsets. Commonly used for
     * margins, padding, borders, and other rectangular insets.
     */
    struct thickness4
    {
    public:
        /** @brief Constructs a thickness with all sides set to zero. */
        constexpr thickness4() noexcept
            : left(0.0f)
            , top(0.0f)
            , right(0.0f)
            , bottom(0.0f)
        {
        }

        /** @brief Constructs a uniform thickness for all sides. */
        constexpr thickness4(float value) noexcept
            : left(value)
            , top(value)
            , right(value)
            , bottom(value)
        {
        }

        /** @brief Constructs a thickness from horizontal and vertical values. */
        constexpr thickness4(float horizontal, float vertical) noexcept
            : left(horizontal)
            , top(vertical)
            , right(horizontal)
            , bottom(vertical)
        {
        }

        /** @brief Constructs a thickness from individual side values. */
        constexpr thickness4(float left, float top, float right, float bottom) noexcept
            : left(left)
            , top(top)
            , right(right)
            , bottom(bottom)
        {
        }

        /** @brief Returns the sum of the left and right sides. */
        [[nodiscard]]
        constexpr float horizontal() const noexcept
        {
            return left + right;
        }

        /** @brief Returns the sum of the top and bottom sides. */
        [[nodiscard]]
        constexpr float vertical() const noexcept
        {
            return top + bottom;
        }

        /** @brief Returns the total size represented by this thickness. */
        [[nodiscard]]
        constexpr math::vec2 size() const noexcept
        {
            return {horizontal(), vertical()};
        }

        /** @brief Returns true if all sides are zero. */
        [[nodiscard]]
        constexpr bool is_zero() const noexcept
        {
            return left == 0.0f && top == 0.0f && right == 0.0f && bottom == 0.0f;
        }

        /** @brief Returns true if all sides have the same value. */
        [[nodiscard]]
        constexpr bool is_uniform() const noexcept
        {
            return left == top && left == right && left == bottom;
        }

        /** @brief Adds two thickness values component-wise. */
        [[nodiscard]]
        constexpr thickness4 operator+(const thickness4& rhs) const noexcept
        {
            return {
                left + rhs.left,
                top + rhs.top,
                right + rhs.right,
                bottom + rhs.bottom
            };
        }

        /** @brief Subtracts two thickness values component-wise. */
        [[nodiscard]]
        constexpr thickness4 operator-(const thickness4& rhs) const noexcept
        {
            return {
                left - rhs.left,
                top - rhs.top,
                right - rhs.right,
                bottom - rhs.bottom
            };
        }

        /** @brief Multiplies all sides by a scalar. */
        [[nodiscard]]
        constexpr thickness4 operator*(float scalar) const noexcept
        {
            return {
                left * scalar,
                top * scalar,
                right * scalar,
                bottom * scalar
            };
        }

        /** @brief Divides all sides by a scalar. */
        [[nodiscard]]
        constexpr thickness4 operator/(float scalar) const noexcept
        {
            return {
                left / scalar,
                top / scalar,
                right / scalar,
                bottom / scalar
            };
        }

        /** @brief Adds another thickness to this one. */
        constexpr thickness4& operator+=(const thickness4& rhs) noexcept
        {
            left += rhs.left;
            top += rhs.top;
            right += rhs.right;
            bottom += rhs.bottom;
            return *this;
        }

        /** @brief Subtracts another thickness from this one. */
        constexpr thickness4& operator-=(const thickness4& rhs) noexcept
        {
            left -= rhs.left;
            top -= rhs.top;
            right -= rhs.right;
            bottom -= rhs.bottom;
            return *this;
        }

        /** @brief Multiplies all sides by a scalar. */
        constexpr thickness4& operator*=(float scalar) noexcept
        {
            left *= scalar;
            top *= scalar;
            right *= scalar;
            bottom *= scalar;
            return *this;
        }

        /** @brief Divides all sides by a scalar. */
        constexpr thickness4& operator/=(float scalar) noexcept
        {
            left /= scalar;
            top /= scalar;
            right /= scalar;
            bottom /= scalar;
            return *this;
        }

        /** @brief Compares two thickness values for equality. */
        [[nodiscard]]
        constexpr bool operator==(const thickness4&) const noexcept = default;

    public:
        float left;
        float top;
        float right;
        float bottom;
    };

    /** @brief Returns the component-wise minimum of two thickness values. */
    [[nodiscard]]
    constexpr thickness4 min(const thickness4& a, const thickness4& b) noexcept
    {
        return {
            std::min(a.left, b.left),
            std::min(a.top, b.top),
            std::min(a.right, b.right),
            std::min(a.bottom, b.bottom)
        };
    }

    /** @brief Returns the component-wise maximum of two thickness values. */
    [[nodiscard]]
    constexpr thickness4 max(const thickness4& a, const thickness4& b) noexcept
    {
        return {
            std::max(a.left, b.left),
            std::max(a.top, b.top),
            std::max(a.right, b.right),
            std::max(a.bottom, b.bottom)
        };
    }

    /** @brief Multiplies all sides by a scalar. */
    [[nodiscard]]
    constexpr thickness4 operator*(float scalar, const thickness4& value) noexcept
    {
        return value * scalar;
    }

} // namespace tavros::geometry
