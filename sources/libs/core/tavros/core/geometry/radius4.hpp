#pragma once

#include <tavros/core/math.hpp>

namespace tavros::geometry
{

    /**
     * @brief Represents corner radii for a rectangle.
     *
     * Stores the radii of the top-left, top-right, bottom-right,
     * and bottom-left corners independently.
     */
    struct radius4
    {
    public:
        /** @brief Constructs a radius with all corners set to zero. */
        constexpr radius4() noexcept
            : top_left(0.0f)
            , top_right(0.0f)
            , bottom_right(0.0f)
            , bottom_left(0.0f)
        {
        }

        /** @brief Constructs a uniform radius for all corners. */
        constexpr radius4(float value) noexcept
            : top_left(value)
            , top_right(value)
            , bottom_right(value)
            , bottom_left(value)
        {
        }

        /** @brief Constructs a radius from individual corner values. */
        constexpr radius4(float top_left, float top_right, float bottom_right, float bottom_left) noexcept
            : top_left(top_left)
            , top_right(top_right)
            , bottom_right(bottom_right)
            , bottom_left(bottom_left)
        {
        }

        /** @brief Returns true if all radii are zero. */
        [[nodiscard]]
        constexpr bool is_zero() const noexcept
        {
            return top_left == 0.0f && top_right == 0.0f && bottom_right == 0.0f && bottom_left == 0.0f;
        }

        /** @brief Returns true if all radii have the same value. */
        [[nodiscard]]
        constexpr bool is_uniform() const noexcept
        {
            return top_left == top_right && top_left == bottom_right && top_left == bottom_left;
        }

        /** @brief Returns the largest radius value. */
        [[nodiscard]]
        constexpr float max() const noexcept
        {
            return std::max(
                std::max(top_left, top_right),
                std::max(bottom_right, bottom_left)
            );
        }

        /** @brief Returns the smallest radius value. */
        [[nodiscard]]
        constexpr float min() const noexcept
        {
            return std::min(
                std::min(top_left, top_right),
                std::min(bottom_right, bottom_left)
            );
        }

        /** @brief Adds two radii component-wise. */
        [[nodiscard]]
        constexpr radius4 operator+(const radius4& rhs) const noexcept
        {
            return {
                top_left + rhs.top_left,
                top_right + rhs.top_right,
                bottom_right + rhs.bottom_right,
                bottom_left + rhs.bottom_left
            };
        }

        /** @brief Subtracts two radii component-wise. */
        [[nodiscard]]
        constexpr radius4 operator-(const radius4& rhs) const noexcept
        {
            return {
                top_left - rhs.top_left,
                top_right - rhs.top_right,
                bottom_right - rhs.bottom_right,
                bottom_left - rhs.bottom_left
            };
        }

        /** @brief Multiplies all radii by a scalar. */
        [[nodiscard]]
        constexpr radius4 operator*(float scalar) const noexcept
        {
            return {
                top_left * scalar,
                top_right * scalar,
                bottom_right * scalar,
                bottom_left * scalar
            };
        }

        /** @brief Divides all radii by a scalar. */
        [[nodiscard]]
        constexpr radius4 operator/(float scalar) const noexcept
        {
            return {
                top_left / scalar,
                top_right / scalar,
                bottom_right / scalar,
                bottom_left / scalar
            };
        }

        /** @brief Adds another radius to this one. */
        constexpr radius4& operator+=(const radius4& rhs) noexcept
        {
            top_left += rhs.top_left;
            top_right += rhs.top_right;
            bottom_right += rhs.bottom_right;
            bottom_left += rhs.bottom_left;
            return *this;
        }

        /** @brief Subtracts another radius from this one. */
        constexpr radius4& operator-=(const radius4& rhs) noexcept
        {
            top_left -= rhs.top_left;
            top_right -= rhs.top_right;
            bottom_right -= rhs.bottom_right;
            bottom_left -= rhs.bottom_left;
            return *this;
        }

        /** @brief Multiplies all radii by a scalar. */
        constexpr radius4& operator*=(float scalar) noexcept
        {
            top_left *= scalar;
            top_right *= scalar;
            bottom_right *= scalar;
            bottom_left *= scalar;
            return *this;
        }

        /** @brief Divides all radii by a scalar. */
        constexpr radius4& operator/=(float scalar) noexcept
        {
            top_left /= scalar;
            top_right /= scalar;
            bottom_right /= scalar;
            bottom_left /= scalar;
            return *this;
        }

        /** @brief Compares two radii for equality. */
        [[nodiscard]]
        constexpr bool operator==(const radius4&) const noexcept = default;

    public:
        float top_left;
        float top_right;
        float bottom_right;
        float bottom_left;
    };

    /** @brief Returns the component-wise minimum of two radii. */
    [[nodiscard]]
    constexpr radius4 min(const radius4& a, const radius4& b) noexcept
    {
        return {
            std::min(a.top_left, b.top_left),
            std::min(a.top_right, b.top_right),
            std::min(a.bottom_right, b.bottom_right),
            std::min(a.bottom_left, b.bottom_left)
        };
    }

    /** @brief Returns the component-wise maximum of two radii. */
    [[nodiscard]]
    constexpr radius4 max(const radius4& a, const radius4& b) noexcept
    {
        return {
            std::max(a.top_left, b.top_left),
            std::max(a.top_right, b.top_right),
            std::max(a.bottom_right, b.bottom_right),
            std::max(a.bottom_left, b.bottom_left)
        };
    }

    /** @brief Multiplies all radii by a scalar. */
    [[nodiscard]]
    constexpr radius4 operator*(float scalar, const radius4& value) noexcept
    {
        return value * scalar;
    }

} // namespace tavros::geometry
