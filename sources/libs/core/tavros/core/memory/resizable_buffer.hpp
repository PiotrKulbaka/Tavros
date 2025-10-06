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
    class resizable_buffer : noncopyable
    {
    public:
        /**
         * @brief Constructs the buffer using the specified allocator.
         * @param alc Pointer to the allocator used for memory management. Must not be null.
         */
        explicit resizable_buffer(allocator* alc) noexcept
            : m_allocator(alc)
        {
            TAV_ASSERT(alc);
        }

        /**
         * @brief Move constructor. Transfers ownership of memory from another buffer.
         */
        resizable_buffer(resizable_buffer&& other) noexcept
            : m_allocator(other.m_allocator)
            , m_storage(other.m_storage)
            , m_size(other.m_size)
        {
            other.m_allocator = nullptr;
            other.m_storage = nullptr;
            other.m_size = 0;
        }

        /**
         * @brief Destructor. Releases any allocated memory.
         */
        ~resizable_buffer() noexcept
        {
            destroy_storage();
        }

        /**
         * @brief Move assignment operator. Releases current memory and takes ownership from another buffer.
         */
        resizable_buffer& operator=(resizable_buffer&& other) noexcept
        {
            if (this != &other) {
                destroy_storage();

                m_allocator = other.m_allocator;
                m_storage = other.m_storage;
                m_size = other.m_size;

                other.m_allocator = nullptr;
                other.m_storage = nullptr;
                other.m_size = 0;
            }
            return *this;
        }

        /**
         * @brief Ensures the buffer has at least the required size.
         *
         * If the current size is smaller than required, the buffer is reallocated.
         */
        void ensure_size(size_t required_size)
        {
            if (m_size < required_size) {
                resize(required_size);
            }
        }

        /**
         * @brief Resizes the buffer to the specified number of elements.
         *
         * Any existing memory is deallocated and replaced with a new allocation.
         */
        void resize(size_t new_size)
        {
            if (m_size != new_size) {
                TAV_ASSERT(m_allocator);

                size_t required_size_bytes = new_size * sizeof(T);
                T*     new_storage = reinterpret_cast<T*>(m_allocator->reallocate(m_storage, required_size_bytes, alignof(T), "resizable_buffer"));

                if (!new_storage) {
                    tavros::core::logger::print(
                        tavros::core::severity_level::error,
                        "resizable_buffer",
                        "Failed to allocate {} bytes",
                        required_size_bytes
                    );
                    if (m_storage) {
                        m_allocator->deallocate(reinterpret_cast<void*>(m_storage));
                    }
                    m_storage = nullptr;
                    m_size = 0;
                } else {
                    m_storage = new_storage;
                    m_size = new_size;
                }
            }
        }

        /**
         * @brief Returns the number of elements allocated.
         */
        size_t size() const noexcept
        {
            return m_size;
        }

        /**
         * @brief Returns a pointer to the underlying buffer.
         */
        T* data() noexcept
        {
            return m_storage;
        }

        /**
         * @brief Returns a pointer to the underlying buffer.
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
                m_size = 0;
            }
        }

    private:
        allocator* m_allocator = nullptr;
        T*         m_storage = nullptr;
        size_t     m_size = 0;
    };

} // namespace tavros::core
