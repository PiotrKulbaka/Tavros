#pragma once

#include <tavros/core/traits.hpp>
#include <tavros/core/fixed_string.hpp>

namespace tavros::renderer
{

    template<class T>
    concept has_is_valid_impl =
        requires(const T& obj) {
            { obj.is_valid_impl() } -> std::same_as<bool>;
        };

    template<class T>
    concept has_hash_impl =
        requires(const T& obj) {
            { obj.hash_impl() } -> std::same_as<size_t>;
        };

    template<class T>
    concept has_equals_impl =
        requires(const T& lhs, const T& rhs) {
            { lhs.equals_impl(rhs) } -> std::same_as<bool>;
        };

    template<class Derived>
    class resource_desc_base
    {
    public:
        resource_desc_base(core::string_view name) noexcept
            : m_name(name)
        {
        }

        ~resource_desc_base() noexcept = default;

        core::string_view name() const noexcept
        {
            return m_name;
        }

        bool is_valid() const noexcept
            requires has_is_valid_impl<Derived>
        {
            return static_cast<const Derived*>(this)->is_valid_impl();
        }

        size_t hash() const noexcept
            requires has_hash_impl<Derived>
        {
            return static_cast<const Derived*>(this)->hash_impl();
        }

        bool operator==(const Derived& other) const noexcept
            requires has_equals_impl<Derived>
        {
            return static_cast<const Derived*>(this)->equals_impl(other);
        }

    private:
        core::fixed_string<255> m_name;
    };

} // namespace tavros::renderer
