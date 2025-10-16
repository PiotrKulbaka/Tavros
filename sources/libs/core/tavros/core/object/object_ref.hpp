#pragma once

#include <tavros/core/object_handle.hpp>
#include <tavros/core/object_pool.hpp>

namespace tavros::core
{

    /**
     * @brief Lightweight non-owning reference to an object stored in an object pool.
     *
     * This class provides safe, read/write access to an object managed by an object pool.
     * It does not manage the lifetime of the object; creation and destruction are handled by the pool.
     * The reference remains valid as long as the pool exists and the referenced object has not been removed.
     */
    template<typename T>
    class object_ref
    {
    public:
        using object_handle = object_handle<T>;
        using object_pool = object_pool<T>;

        /**
         * @brief Constructs an invalid object reference.
         */
        explicit object_ref() noexcept
            : m_pool(nullptr)
            , m_handle(object_handle::invalid())
        {
        }

        /**
         * @brief Constructs an object reference from a pool and an object handle.
         * @param pool Pointer to the object pool that owns the object.
         * @param handle Handle identifying the object in the pool.
         */
        explicit object_ref(object_pool* pool, object_handle handle) noexcept
            : m_pool(pool)
            , m_handle(handle)
        {
        }

        /**
         * @brief Returns a pointer to the referenced object, or nullptr if invalid.
         * @return Pointer to the object, or nullptr if the pool or handle is invalid.
         */
        [[nodiscard]] T* get() noexcept
        {
            return m_pool ? m_pool->try_get(m_handle) : nullptr;
        }

        /**
         * @brief Returns a const pointer to the referenced object, or nullptr if invalid.
         * @return Const pointer to the object, or nullptr if the pool or handle is invalid.
         */
        [[nodiscard]] const T* get() const noexcept
        {
            return m_pool ? m_pool->try_get(m_handle) : nullptr;
        }

        /**
         * @brief Provides direct access to the object.
         * @warning No validity checks are performed; call @ref valid() before using.
         */
        [[nodiscard]] T* operator->() noexcept
        {
            return get();
        }

        /**
         * @brief Provides const access to the object.
         * @warning No validity checks are performed; call @ref valid() before using.
         */
        [[nodiscard]] const T* operator->() const noexcept
        {
            return get();
        }

        /**
         * @brief Checks whether the reference currently points to a valid object.
         * @return True if the pool exists and the object handle is valid.
         */
        [[nodiscard]] bool valid() const noexcept
        {
            return m_pool && m_pool->exists(m_handle);
        }

        /**
         * @brief Allows checking validity in boolean context.
         * @example if (ref) { ... }
         */
        explicit operator bool() const noexcept
        {
            return valid();
        }

        /**
         * @brief Returns the handle associated with this reference.
         */
        [[nodiscard]] object_handle handle() const noexcept
        {
            return m_handle;
        }

    private:
        object_pool*  m_pool;
        object_handle m_handle;
    };

} // namespace tavros::core
