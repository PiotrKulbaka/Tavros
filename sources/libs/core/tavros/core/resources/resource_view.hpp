#pragma once

#include <tavros/core/resources/resource_handle.hpp>
#include <tavros/core/resources/resource_pool.hpp>

namespace tavros::core
{

    /**
     * @brief Lightweight non-owning view for a resource stored in a resource pool.
     *
     * This class provides safe, read-only access to a resource managed by a resource pool.
     * It does not manage the lifetime of the resource itself � the pool is responsible for creation
     * and destruction. The view is considered valid as long as the pool exists and the resource
     * has not been removed.
     */
    template<typename T>
    class resource_view
    {
    public:
        using resource_handle = core::resource_handle<T>;
        using resource_pool = core::resource_pool<T>;

        /**
         * @brief Constructs an invalid resource view.
         */
        explicit resource_view() noexcept
            : m_pool(nullptr)
            , m_handle(resource_handle::invalid())
        {
        }

        /**
         * @brief Constructs a resource view from a pool and a resource handle.
         * @param pool Pointer to the resource pool that owns the resource.
         * @param handle Handle identifying the resource in the pool.
         */
        explicit resource_view(resource_pool* pool, resource_handle handle) noexcept
            : m_pool(pool)
            , m_handle(handle)
        {
        }

        /**
         * @brief Returns a pointer to the resource, or nullptr if the pool or resource is not valid.
         * @return Pointer to the resource.
         */
        [[nodiscard]] const T* get() const noexcept
        {
            return m_pool ? m_pool->try_get(m_handle) : nullptr;
        }

        /**
         * @brief Returns a pointer to the resource, or nullptr if the pool or resource is not valid.
         * @return Pointer to the resource.
         */
        [[nodiscard]] const T* operator->() const noexcept
        {
            return get();
        }

        /**
         * @brief Checks whether the resource is currently valid (exists in the pool).
         * @return True if the resource exists and pool is valid; false otherwise.
         */
        [[nodiscard]] bool valid() const noexcept
        {
            return m_pool && m_pool->exists(m_handle);
        }

        /**
         * @brief Returns resource handle.
         */
        [[nodiscard]] resource_handle handle() const noexcept
        {
            return m_handle;
        }

    private:
        resource_pool*  m_pool;
        resource_handle m_handle;
    };

} // namespace tavros::core
