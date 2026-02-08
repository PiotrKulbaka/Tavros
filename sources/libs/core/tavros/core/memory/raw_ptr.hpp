#pragma once

#include <tavros/core/types.hpp>

namespace tavros::core
{

    /**
     * @brief Lightweight wrapper around a raw pointer.
     *
     * raw_ptr<T> is a non-owning pointer wrapper that supports move semantics.
     * Moving a raw_ptr will transfer the pointer and set the source to nullptr.
     *
     * @tparam T Type of the pointed object.
     *
     * @note Does NOT delete or manage the lifetime of the object.
     */
    template<class T>
    class raw_ptr
    {
    public:
        /**
         * @brief Default constructor. Pointer is nullptr.
         */
        raw_ptr() noexcept
            : m_ptr(nullptr)
        {
        }

        /**
         * @brief Constructs wrapper from a raw pointer.
         * @param ptr Pointer to wrap.
         */
        raw_ptr(T* ptr) noexcept
            : m_ptr(ptr)
        {
        }

        /**
         * @brief Constructs wrapper from nullptr.
         */
        raw_ptr(std::nullptr_t) noexcept
            : m_ptr(nullptr)
        {
        }

        /**
         * @brief Copy constructor. Simply copies the pointer.
         */
        raw_ptr(const raw_ptr& other) noexcept = default;

        /**
         * @brief Move constructor. Transfers ownership, source becomes nullptr.
         */
        raw_ptr(raw_ptr&& other) noexcept
            : m_ptr(other.m_ptr)
        {
            other.m_ptr = nullptr;
        }

        /**
         * @brief Destructor. Does not delete the object.
         */
        ~raw_ptr() noexcept
        {
            m_ptr = nullptr;
        }

        /**
         * @brief Copy assignment. Simply copies the pointer.
         */
        raw_ptr& operator=(const raw_ptr& other) noexcept = default;

        /**
         * @brief Move assignment. Transfers ownership, source becomes nullptr.
         */
        raw_ptr& operator=(raw_ptr&& other) noexcept
        {
            if (this != &other) {
                m_ptr = other.m_ptr;
                other.m_ptr = nullptr;
            }
            return *this;
        }

        /**
         * @brief Checks if the pointer is not nullptr.
         */
        operator bool() const noexcept
        {
            return m_ptr != nullptr;
        }

        /**
         * @brief Returns the raw pointer.
         */
        T* get() noexcept
        {
            return m_ptr;
        }

        /**
         * @brief Returns the raw pointer (const version).
         */
        const T* get() const noexcept
        {
            return m_ptr;
        }

        /**
         * @brief Access the pointed object.
         */
        T* operator->() noexcept
        {
            return get();
        }

        /**
         * @brief Access the pointed object (const version).
         */
        const T* operator->() const noexcept
        {
            return get();
        }

        /**
         * @brief Dereference operator.
         */
        T& operator*() noexcept
        {
            return *get();
        }

        /**
         * @brief Dereference operator (const version).
         */
        const T& operator*() const noexcept
        {
            return *get();
        }

    private:
        T* m_ptr;
    };


    /**
     * @brief Comparison operators for raw_ptr.
     */
    template<class T>
    bool operator<=>(const raw_ptr<T>& l, const raw_ptr<T>& r) noexcept
    {
        return l.get() <=> r.get();
    }

    template<class T>
    bool operator==(const raw_ptr<T>& l, std::nullptr_t) noexcept
    {
        return l.get() == nullptr;
    }

    template<class T>
    bool operator!=(const raw_ptr<T>& l, std::nullptr_t) noexcept
    {
        return l.get() != nullptr;
    }

    template<class T>
    bool operator==(std::nullptr_t, const raw_ptr<T>& r) noexcept
    {
        return nullptr == r.get();
    }

    template<class T>
    bool operator!=(std::nullptr_t, const raw_ptr<T>& r) noexcept
    {
        return nullptr != r.get();
    }

} // namespace tavros::core
