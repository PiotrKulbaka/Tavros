#pragma once

#include <tavros/core/math/functions/basic_math.hpp>

namespace tavros::math
{

    /**
     * @brief Represents an angle stored internally in radians.
     *
     * The angle value is always stored as radians. Conversion to degrees
     * or turns is performed when requested.
     */
    class angle
    {
    public:
        /**
         * @brief Constructs an angle from a value in radians.
         *
         * @param radians Angle value in radians.
         */
        constexpr explicit angle(float radians) noexcept
            : m_radians(radians)
        {
        }

        /**
         * @brief Returns the angle in radians.
         *
         * @return Angle value in radians.
         */
        constexpr float radians() const noexcept
        {
            return m_radians;
        }

        /**
         * @brief Returns the angle in degrees.
         *
         * @return Angle value in degrees.
         */
        constexpr float degrees() const noexcept
        {
            return m_radians * 180.0f / k_pi<float>;
        }

        /**
         * @brief Returns the angle in turns.
         *
         * One full turn is equal to 360 degrees or 2pi radians.
         *
         * @return Angle value in turns.
         */
        constexpr float turns() const noexcept
        {
            return m_radians / k_two_pi<float>;
        }

        /** @brief Returns the negated angle. */
        constexpr angle operator-() const noexcept
        {
            return angle(-m_radians);
        }

        /** @brief Returns the angle unchanged. */
        constexpr angle operator+() const noexcept
        {
            return *this;
        }

        /** @brief Adds another angle. */
        constexpr angle& operator+=(angle other) noexcept
        {
            m_radians += other.m_radians;
            return *this;
        }

        /** @brief Subtracts another angle. */
        constexpr angle& operator-=(angle other) noexcept
        {
            m_radians -= other.m_radians;
            return *this;
        }

        /** @brief Multiplies the angle by a scalar. */
        constexpr angle& operator*=(float scalar) noexcept
        {
            m_radians *= scalar;
            return *this;
        }

        /** @brief Divides the angle by a scalar. */
        constexpr angle& operator/=(float scalar) noexcept
        {
            m_radians /= scalar;
            return *this;
        }

        /**
         * @brief Normalizes the angle to the [-pi, pi) range.
         *
         * The angle is modified in-place. The returned value specifies
         * how many complete turns were removed from the original angle
         * during normalization.
         *
         * @return Number of complete turns removed during normalization.
         */
        int32 normalize() noexcept
        {
            float turns = std::floor((m_radians + k_pi<float>) / k_two_pi<float>);
            m_radians -= turns * k_two_pi<float>;
            return static_cast<int32>(turns);
        }

        constexpr bool operator==(angle other) const noexcept
        {
            return m_radians == other.m_radians;
        }

        constexpr bool operator!=(angle other) const noexcept
        {
            return m_radians != other.m_radians;
        }

        constexpr bool operator<(angle other) const noexcept
        {
            return m_radians < other.m_radians;
        }

        constexpr bool operator<=(angle other) const noexcept
        {
            return m_radians <= other.m_radians;
        }

        constexpr bool operator>(angle other) const noexcept
        {
            return m_radians > other.m_radians;
        }

        constexpr bool operator>=(angle other) const noexcept
        {
            return m_radians >= other.m_radians;
        }

    private:
        float m_radians = 0.0f;
    };


    /** @brief Adds two angles. */
    constexpr angle operator+(angle lhs, angle rhs) noexcept
    {
        lhs += rhs;
        return lhs;
    }

    /** @brief Subtracts one angle from another. */
    constexpr angle operator-(angle lhs, angle rhs) noexcept
    {
        lhs -= rhs;
        return lhs;
    }

    /** @brief Multiplies an angle by a scalar. */
    constexpr angle operator*(angle value, float scalar) noexcept
    {
        value *= scalar;
        return value;
    }

    /** @brief Multiplies an angle by a scalar. */
    constexpr angle operator*(float scalar, angle value) noexcept
    {
        value *= scalar;
        return value;
    }

    /** @brief Divides an angle by a scalar. */
    constexpr angle operator/(angle value, float scalar) noexcept
    {
        value /= scalar;
        return value;
    }


    namespace literals
    {

        /** @brief Creates an angle from degrees. */
        constexpr angle operator""_deg(long double value) noexcept
        {
            return angle(static_cast<float>(value) / 180.0f * k_pi<float>);
        }

        /** @brief Creates an angle from radians. */
        constexpr angle operator""_rad(long double value) noexcept
        {
            return angle(static_cast<float>(value));
        }

        /** @brief Creates an angle from turns. */
        constexpr angle operator""_turn(long double value) noexcept
        {
            return angle(static_cast<float>(value) * k_two_pi<float>);
        }

    } /* namespace literals */

} // namespace tavros::math
