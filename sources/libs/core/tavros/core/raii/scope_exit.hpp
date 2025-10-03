#pragma once

#include <tavros/core/noncopyable.hpp>
#include <functional>

namespace tavros::core
{

    /**
     * @brief Utility class for executing a callback when leaving a scope.
     *
     * This class implements the RAII. It ensures that a provided callback
     * (cleanup function) will be executed when the object goes out of scope, unless
     * explicitly released with @ref release().
     *
     * @tparam ScopeExit The type of the cleanup callback (e.g., a lambda or functor).
     */
    template<class ScopeExit>
    class scope_exit : public noncopyable
    {
    public:
        /**
         * @brief Constructs a scope_exit with a cleanup callback.
         *
         * @param exit_callback The callback to be executed on scope exit.
         */
        explicit scope_exit(ScopeExit&& exit_callback) noexcept
            : m_exit_callback(std::forward<ScopeExit>(exit_callback))
            , m_active(true)
        {
        }

        /**
         * @brief Move constructor. Transfers ownership of the callback.
         */
        scope_exit(scope_exit&& other) noexcept
            : m_exit_callback(std::move(other.m_exit_callback))
            , m_active(other.m_active)
        {
            other.release();
        }

        /**
         * @brief Destructor. Executes the cleanup callback if still active.
         */
        ~scope_exit()
        {
            if (m_active) {
                m_exit_callback();
                m_active = false;
            }
        }

        /**
         * @brief Move assignment operator. Releases the current callback
         *        and takes ownership of another.
         */
        scope_exit& operator=(scope_exit&& other) noexcept
        {
            if (this != &other) {
                release();

                m_active = other.m_active;
                m_exit_callback = std::move(other.m_exit_callback);
                other.release();
            }

            return *this;
        }

        /**
         * @brief Prevents the callback from being executed on destruction.
         */
        void release() noexcept
        {
            m_active = false;
        }

    private:
        ScopeExit m_exit_callback;
        bool      m_active;
    };

    /**
     * @brief Helper function to create a scope_exit with automatic type deduction.
     *
     * @tparam ScopeExit The type of the cleanup callback (deduced automatically).
     * @param exit The callback to be executed when the scope is exited.
     * @return A scope_exit instance managing the callback.
     */
    template<class ScopeExit>
    scope_exit<ScopeExit> make_scope_exit(ScopeExit exit)
    {
        return scope_exit<ScopeExit>(std::move(exit));
    }

} // namespace tavros::core
