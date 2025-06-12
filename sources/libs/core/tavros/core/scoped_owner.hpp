#pragma once

#include <tavros/core/noncopyable.hpp>
#include <functional>
#include <memory>  // for std::addressof
#include <utility> // for std::exchange

namespace tavros::core
{

    /**
     * @brief A scoped owner class template that manages ownership of a resource with a customizable cleanup callback.
     *
     * This class uniquely owns a resource of type T and calls a user-provided cleanup function upon destruction,
     * unless ownership is released explicitly. It supports move semantics but disallows copying.
     *
     * @tparam T The type of the owned resource.
     * @tparam ScopeExit The type of the cleanup function.
     */
    template<class T, class ScopeExit>
    class scoped_owner : public noncopyable
    {
    public:
        using value_type = T;
        using scope_exit_type = ScopeExit;

    public:
        /**
         * @brief Default constructor. Constructs an empty scoped_owner without resource or cleanup callback.
         */
        scoped_owner()
        {
        }

        /**
         * @brief Constructs scoped_owner with a resource and a cleanup callback.
         *
         * @param value The resource to own (moved into internal storage).
         * @param exit The cleanup callback to call on destruction if not released.
         */
        scoped_owner(value_type value, scope_exit_type exit)
            : m_value(std::move(value))
            , m_exit_callback(std::move(exit))
        {
        }

        /**
         * @brief Destructor. Calls the cleanup callback with the owned resource if still owned.
         */
        ~scoped_owner()
        {
            cleanup();
        }

        /**
         * @brief Move constructor. Transfers ownership and cleanup callback from another instance.
         */
        scoped_owner(scoped_owner&& other) noexcept
            : m_value(std::move(other.m_value))
            , m_has_value(other.m_has_value)
            , m_exit_callback(std::move(other.m_exit_callback))
        {
            other.m_has_value = false;
        }

        /**
         * @brief Move assignment operator. Cleans up current owned resource and transfers ownership and cleanup callback.
         */
        scoped_owner& operator=(scoped_owner&& other) noexcept
        {
            if (this != &other) {
                cleanup();

                m_value = std::move(other.m_value);
                m_exit_callback = std::move(other.m_exit_callback);
                other.m_has_value = false;
            }

            return *this;
        }

        /**
         * @brief Releases ownership of the resource without calling the cleanup callback.
         *
         * After this call, the scoped_owner will no longer manage the resource or invoke the callback.
         *
         * @return The previously owned resource.
         */
        value_type release() noexcept
        {
            if (!m_has_value) {
                return value_type{};
            }
            m_has_value = false;
            return std::exchange(m_value, value_type{});
        }

        /**
         * @brief Checks whether the scoped_owner currently holds a valid value.
         *
         * @return true if scoped_owner owns a valid resource (i.e., has a non-empty value), false otherwise.
         */
        bool has_value() const noexcept
        {
            return m_has_value;
        }

        /**
         * @brief Gets a const reference to the owned resource.
         *
         * @return Const reference to the resource.
         */
        constexpr const value_type& get() const noexcept
        {
            return m_value;
        }

        /**
         * @brief Gets a mutable reference to the owned resource.
         *
         * @return Mutable reference to the resource.
         */
        constexpr value_type& get() noexcept
        {
            return m_value;
        }

        /**
         * @brief Dereference operator for mutable access.
         *
         * @return Mutable reference to the resource.
         */
        constexpr value_type& operator*() noexcept
        {
            return get();
        }

        /**
         * @brief Dereference operator for const access.
         *
         * @return Const reference to the resource.
         */
        constexpr const value_type& operator*() const noexcept
        {
            return get();
        }

        /**
         * @brief Member access operator for mutable access.
         *
         * @return Pointer to the resource.
         */
        constexpr value_type* operator->() noexcept
        {
            return std::addressof(m_value);
        }

        /**
         * @brief Member access operator for const access.
         *
         * @return Const pointer to the resource.
         */
        constexpr const value_type* operator->() const noexcept
        {
            return std::addressof(m_value);
        }

    private:
        /**
         * @brief Internal helper to invoke the cleanup callback if set.
         *
         * After calling the callback, disables it to prevent repeated calls.
         */
        void cleanup()
        {
            if (m_has_value) {
                m_exit_callback(std::move(m_value));
                m_has_value = false;
            }
        }

    private:
        value_type      m_value;
        bool            m_has_value = false;
        scope_exit_type m_exit_callback;
    };

} // namespace tavros::core
