#pragma once

#include <tavros/core/object_handle.hpp>
#include <tavros/core/object_pool.hpp>

namespace tavros::core
{

    /**
     * @brief Lightweight non-owning view for a object stored in a object pool.
     *
     * This class provides safe, read-only access to a object managed by a object pool.
     * It does not manage the lifetime of the object itself, and the pool is responsible for creation
     * and destruction. The view is considered valid as long as the pool exists and the object
     * has not been removed.
     */
    template<typename T>
    class object_view
    {
    public:
        using object_handle = object_handle<T>;
        using object_pool = object_pool<T>;

        /**
         * @brief Constructs an invalid object view.
         */
        explicit object_view() noexcept
            : m_pool(nullptr)
            , m_handle(object_handle::invalid())
        {
        }

        /**
         * @brief Constructs a object view from a pool and a object handle.
         * @param pool Pointer to the object pool that owns the object.
         * @param handle Handle identifying the object in the pool.
         */
        explicit object_view(object_pool* pool, object_handle handle) noexcept
            : m_pool(pool)
            , m_handle(handle)
        {
        }

        /**
         * @brief Returns a pointer to the object, or nullptr if the pool or object is not valid.
         * @return Pointer to the object.
         */
        [[nodiscard]] const T* get() const noexcept
        {
            return m_pool ? m_pool->try_get(m_handle) : nullptr;
        }

        /**
         * @brief Returns a pointer to the object, or nullptr if the pool or object is not valid.
         * @return Pointer to the object.
         */
        [[nodiscard]] const T* operator->() const noexcept
        {
            return get();
        }

        /**
         * @brief Checks whether the object is currently valid (exists in the pool).
         * @return True if the object exists and pool is valid; false otherwise.
         */
        [[nodiscard]] bool valid() const noexcept
        {
            return m_pool && m_pool->exists(m_handle);
        }

        /**
         * @brief Returns object handle.
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
