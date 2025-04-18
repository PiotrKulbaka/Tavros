#pragma once

#include <tavros/core/types.hpp>
#include <utility>
#include <type_traits>

namespace tavros::core
{
    /**
     * @brief Small pimpl container with static memory.
     *
     * @details Does not allocate. Layout is controlled by template parameters.
     * Use with caution: T must fit exactly in [Size, Alignment].
     */
    template<class T, std::size_t Size, std::size_t Alignment>
    class pimpl
    {
    public:
        /**
         * @brief Construct a new pimpl object.
         */
        template<class... Args>
        explicit pimpl(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args&&...>)
        {
            new (get()) T(std::forward<Args>(args)...);
        }

        /**
         * @brief Destroy the pimpl object.
         */
        ~pimpl() noexcept
        {
            validate<sizeof(T), alignof(T)>();
            get()->~T();
        }

        /**
         * @brief Move construct the pimpl object.
         */
        pimpl(pimpl&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
        {
            new (get()) T(std::move(*other.get()));
        }

        /**
         * @brief Move assign the contents from another pimpl.
         */
        pimpl& operator=(pimpl&& other) noexcept(std::is_nothrow_copy_assignable_v<T>)
        {
            get()->~T();
            new (get()) T(std::move(*other.get()));
            return *this;
        }

        /**
         * @brief Copy construct the pimpl object.
         */
        pimpl(const pimpl& other) noexcept(std::is_nothrow_copy_constructible_v<T>)
        {
            new (get()) T(*other.get());
        }

        /**
         * @brief Copy assign the contents from another pimpl.
         */
        pimpl& operator=(const pimpl& other) noexcept(std::is_nothrow_copy_assignable_v<T>)
        {
            *get() = *other.get();
            return *this;
        }

        /**
         * @brief Returns a pointer to the underlying object.
         */
        T* get() noexcept
        {
            return reinterpret_cast<T*>(&storage);
        }

        /**
         * @brief Returns a const pointer to the underlying object.
         */
        const T* get() const noexcept
        {
            return reinterpret_cast<const T*>(&storage);
        }

        /**
         * @brief Returns a pointer to the underlying object.
         */
        T* operator->() noexcept
        {
            return get();
        }

        /**
         * @brief Returns a const pointer to the underlying object.
         */
        const T* operator->() const noexcept
        {
            return get();
        }

        /**
         * @brief Get the pimpl object.
         */
        T& operator*() noexcept
        {
            return *get();
        }

        /**
         * @brief Get the const pimpl object.
         */
        const T& operator*() const noexcept
        {
            return *get();
        }

    private:
        template<std::size_t ActualSize, std::size_t ActualAlignment>
        static void validate() noexcept
        {
            static_assert(Size == ActualSize, "Size and sizeof(T) mismatch");
            static_assert(Alignment == ActualAlignment, "Alignment and alignof(T) mismatch");
        }

    private:
        alignas(Alignment) uint8 storage[Size];
    }; // class pimpl
} // namespace tavros::core
