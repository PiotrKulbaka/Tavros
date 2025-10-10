#pragma once

#include <tavros/core/noncopyable.hpp>
#include <tavros/core/debug/assert.hpp>
#include <tavros/core/logger/logger.hpp>
#include <tavros/core/memory/allocator.hpp>

namespace tavros::core
{

    /**
     * @brief A lightweight dynamically resizable buffer for raw memory management.
     *
     * This class manages a contiguous block of memory using a provided allocator.
     * It can automatically reallocate to a larger size when needed.
     *
     * Copying is disabled, but moving is supported.
     */
    template<class T>
    class dynamic_buffer : noncopyable
    {
    public:
        /**
         * @brief Constructs the buffer using the specified allocator.
         * @param alc Pointer to the allocator used for memory management. Must not be null.
         */
        explicit dynamic_buffer(allocator* alc) noexcept
            : m_allocator(alc)
        {
            TAV_ASSERT(alc);
        }

        /**
         * @brief Move constructor. Transfers ownership of the allocated memory from another buffer.
         */
        dynamic_buffer(dynamic_buffer&& other) noexcept
            : m_allocator(other.m_allocator)
            , m_storage(other.m_storage)
            , m_capacity(other.m_capacity)
        {
            other.m_allocator = nullptr;
            other.m_storage = nullptr;
            other.m_capacity = 0;
        }

        /**
         * @brief Destructor. Releases any allocated memory.
         */
        ~dynamic_buffer() noexcept
        {
            destroy_storage();
        }

        /**
         * @brief Move assignment operator. Releases current memory and takes ownership from another buffer.
         */
        dynamic_buffer& operator=(dynamic_buffer&& other) noexcept
        {
            if (this != &other) {
                destroy_storage();

                m_allocator = other.m_allocator;
                m_storage = other.m_storage;
                m_capacity = other.m_capacity;

                other.m_allocator = nullptr;
                other.m_storage = nullptr;
                other.m_capacity = 0;
            }
            return *this;
        }

        /**
         * @brief Ensures that the buffer has at least the specified capacity.
         *
         * If the current capacity is smaller than the required one,
         * the buffer is reallocated to the new size.
         *
         * @param required_capacity Minimum number of elements the buffer must be able to hold.
         */
        void reserve(size_t required_capacity)
        {
            if (m_capacity < required_capacity) {
                resize(required_capacity);
            }
        }

        /**
         * @brief Resizes the buffer to the specified number of elements.
         *
         * Any existing memory is reallocated. If allocation fails, the buffer becomes empty.
         *
         * @param new_capacity Number of elements to allocate space for.
         */
        void resize(size_t new_capacity)
        {
            if (m_capacity != new_capacity) {
                TAV_ASSERT(m_allocator);

                size_t required_size_bytes = new_capacity * sizeof(T);
                T*     new_storage = reinterpret_cast<T*>(
                    m_allocator->reallocate(m_storage, required_size_bytes, alignof(T), "dynamic_buffer")
                );

                if (!new_storage) {
                    tavros::core::logger::print(
                        tavros::core::severity_level::error,
                        "dynamic_buffer",
                        "Failed to allocate {} bytes",
                        required_size_bytes
                    );

                    if (m_storage) {
                        m_allocator->deallocate(reinterpret_cast<void*>(m_storage));
                    }

                    m_storage = nullptr;
                    m_capacity = 0;
                } else {
                    m_storage = new_storage;
                    m_capacity = new_capacity;
                }
            }
        }

        /**
         * @brief Returns the number of elements currently allocated.
         */
        size_t capacity() const noexcept
        {
            return m_capacity;
        }

        /**
         * @brief Returns a pointer to the underlying buffer memory.
         */
        T* data() noexcept
        {
            return m_storage;
        }

        /**
         * @brief Returns a const pointer to the underlying buffer memory.
         */
        const T* data() const noexcept
        {
            return m_storage;
        }

    private:
        /**
         * @brief Frees the allocated memory, if any.
         */
        void destroy_storage()
        {
            if (m_storage && m_allocator) {
                m_allocator->deallocate(m_storage);
                m_storage = nullptr;
                m_capacity = 0;
            }
        }

    private:
        allocator* m_allocator = nullptr;
        T*         m_storage = nullptr;
        size_t     m_capacity = 0;
    };

} // namespace tavros::core
