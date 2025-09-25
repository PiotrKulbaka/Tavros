#pragma once

#include <optional>
#include <tavros/core/types.hpp>

namespace tavros::core
{

    /**
     * ref_ptr - shared_ptr-like smart pointer without ownership and allocations.
     *
     * This class wraps a raw pointer and optionally maintains a reference count.
     * It does not manage the lifetime of the object itself; the memory for both
     * the object and the reference counter is owned and managed by the caller.
     *
     * The reference count can be incremented and decremented atomically, making
     * it safe to use across threads for counting purposes. Destruction of the
     * underlying object is the responsibility of external code, not ref_ptr.
     *
     * Typical use cases include GPU resources or other objects where lifetime
     * management is handled by a higher-level system (e.g., render_system),
     * but multiple references need to be tracked safely and efficiently.
     */
    template<class T>
    class ref_ptr
    {
    public:
        ref_ptr() noexcept
            : m_ptr(nullptr)
            , m_ref_counter(nullptr)
        {
        }

        ref_ptr(T* ptr) noexcept
            : m_ptr(ptr)
            , m_ref_counter(nullptr)
        {
        }

        ref_ptr(T* ptr, atomic_size_t* ref_counter) noexcept
            : m_ptr(ptr)
            , m_ref_counter(ref_counter)
        {
            add_ref();
        }

        ref_ptr(std::nullptr_t) noexcept
            : m_ptr(nullptr)
            , m_ref_counter(nullptr)
        {
        }


        ref_ptr(const ref_ptr& other) noexcept
            : m_ptr(other.m_ptr)
            , m_ref_counter(other.m_ref_counter)
        {
            add_ref();
        }

        ref_ptr(ref_ptr&& other) noexcept
            : m_ptr(other.m_ptr)
            , m_ref_counter(other.m_ref_counter)
        {
            other.m_ptr = nullptr;
            other.m_ref_counter = nullptr;
        }

        ~ref_ptr() noexcept
        {
            release();
        }


        ref_ptr& operator=(const ref_ptr& other) noexcept
        {
            if (this != &other) {
                // Release old data
                release();
                m_ptr = other.m_ptr;
                m_ref_counter = other.m_ref_counter;
                // Add ref to new data
                add_ref();
            }
            return *this;
        }

        ref_ptr& operator=(ref_ptr&& other) noexcept
        {
            if (this != &other) {
                // Release old data
                release();
                m_ptr = other.m_ptr;
                m_ref_counter = other.m_ref_counter;
                other.m_ptr = nullptr;
                other.m_ref_counter = nullptr;
            }
            return *this;
        }


        operator bool() const noexcept
        {
            return m_ptr != nullptr;
        }

        T* get() noexcept
        {
            return m_ptr;
        }

        const T* get() const noexcept
        {
            return m_ptr;
        }

        T* operator->() noexcept
        {
            return get();
        }

        const T* operator->() const noexcept
        {
            return get();
        }

        T& operator*() noexcept
        {
            return *get();
        }

        const T& operator*() const noexcept
        {
            return *get();
        }

        bool has_counter() const noexcept
        {
            return !!m_ref_counter;
        }

        size_t use_count() const noexcept
        {
            return m_ref_counter ? m_ref_counter->load() : 0;
        }

    private:
        void add_ref() noexcept
        {
            if (m_ref_counter) {
                m_ref_counter->fetch_add(1, std::memory_order_relaxed);
            }
        }

        void release() noexcept
        {
            if (m_ref_counter) {
                m_ref_counter->fetch_sub(1, std::memory_order_relaxed);
            }
        }

    private:
        T*             m_ptr;
        atomic_size_t* m_ref_counter;
    };


    template<class T>
    bool operator<=>(const ref_ptr<T>& l, const ref_ptr<T>& r) noexcept
    {
        return l.get() <=> r.get();
    }

    template<class T>
    bool operator==(const ref_ptr<T>& l, std::nullptr_t) noexcept
    {
        return l.get() == nullptr;
    }

    template<class T>
    bool operator!=(const ref_ptr<T>& l, std::nullptr_t) noexcept
    {
        return l.get() != nullptr;
    }

    template<class T>
    bool operator==(std::nullptr_t, const ref_ptr<T>& r) noexcept
    {
        return nullptr == r.get();
    }

    template<class T>
    bool operator!=(std::nullptr_t, const ref_ptr<T>& r) noexcept
    {
        return nullptr != r.get();
    }

} // namespace tavros::core
