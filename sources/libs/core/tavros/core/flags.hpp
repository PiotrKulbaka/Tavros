#pragma once

#include <type_traits>
#include <tavros/core/types.hpp>

namespace tavros::core
{

    template<typename T>
    class flags
    {
    private:
        static_assert(std::is_enum_v<T>, "flags<T>: T must be an enum type");

        using underlying_t = std::underlying_type_t<T>;

    public:
        constexpr flags() noexcept
            : m_bits(0)
        {
        }

        constexpr flags(T flag) noexcept
            : m_bits(static_cast<underlying_t>(flag))
        {
        }

        constexpr explicit flags(underlying_t bits) noexcept
            : m_bits(bits)
        {
        }

        flags& operator|=(flags other) noexcept
        {
            m_bits |= other.m_bits;
            return *this;
        }

        flags& operator&=(flags other) noexcept
        {
            m_bits &= other.m_bits;
            return *this;
        }

        flags& operator^=(flags other) noexcept
        {
            m_bits ^= other.m_bits;
            return *this;
        }

        constexpr bool operator==(flags other) const noexcept
        {
            return m_bits == other.m_bits;
        }

        constexpr bool operator!=(flags other) const noexcept
        {
            return m_bits != other.m_bits;
        }

        constexpr bool operator==(bool other) const noexcept
        {
            return static_cast<bool>(m_bits) == other;
        }

        constexpr bool operator!=(bool other) const noexcept
        {
            return static_cast<bool>(m_bits) != other;
        }

        constexpr underlying_t bits() const noexcept
        {
            return m_bits;
        }

        constexpr flags operator~() const noexcept
        {
            return flags(~m_bits);
        }

        friend constexpr flags operator|(flags lhs, flags rhs) noexcept
        {
            return flags(lhs.m_bits | rhs.m_bits);
        }

        friend constexpr flags operator&(flags lhs, flags rhs) noexcept
        {
            return flags(lhs.m_bits & rhs.m_bits);
        }

        friend constexpr flags operator^(flags lhs, flags rhs) noexcept
        {
            return flags(lhs.m_bits ^ rhs.m_bits);
        }

        constexpr bool has_flag(flags flag) const noexcept
        {
            return (m_bits & flag.m_bits) == flag.m_bits;
        }

    private:
        underlying_t m_bits;
    };

    template<typename T>
    flags(T) -> flags<T>;
} // namespace tavros::core