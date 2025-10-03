#pragma once

#include <tavros/core/containers/sapn.hpp>
#include <tavros/core/debug/assert.hpp>

#include <stdexcept>
#include <initializer_list>
#include <iterator>
#include <array>

namespace tavros::core
{

    template<typename T, std::size_t N>
    class static_vector
    {
    public:
        using iterator = T*;
        using const_iterator = const T*;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    public:
        static_vector() noexcept
            : m_data({})
            , m_size(0)
        {
        }

        constexpr static_vector(std::initializer_list<T> init) noexcept
        {
            TAV_ASSERT(init.size() <= N);
            for (const auto& value : init) {
                emplace_back(value);
            }
        }

        constexpr static_vector(span<const T> init) noexcept
        {
            TAV_ASSERT(init.size() <= N);
            for (const auto& value : init) {
                emplace_back(value);
            }
        }

        constexpr void swap(static_vector& other) noexcept
        {
            m_data.swap(other.m_data);
            auto sz = m_size;
            m_size = other.m_size;
            other.m_size = sz;
        }

        constexpr iterator begin() noexcept
        {
            return m_data.data();
        }

        constexpr const_iterator begin() const noexcept
        {
            return m_data.data();
        }

        constexpr iterator end() noexcept
        {
            return m_data.data() + m_size;
        }

        constexpr const_iterator end() const noexcept
        {
            return m_data.data() + m_size;
        }

        constexpr reverse_iterator rbegin() noexcept
        {
            return reverse_iterator(end());
        }

        constexpr const_reverse_iterator rbegin() const noexcept
        {
            return const_reverse_iterator(end());
        }

        constexpr reverse_iterator rend() noexcept
        {
            return reverse_iterator(begin());
        }

        constexpr const_reverse_iterator rend() const noexcept
        {
            return const_reverse_iterator(begin());
        }

        constexpr const_iterator cbegin() const noexcept
        {
            return begin();
        }

        constexpr const_iterator cend() const noexcept
        {
            return end();
        }

        constexpr const_reverse_iterator crbegin() const noexcept
        {
            return rbegin();
        }

        constexpr const_reverse_iterator crend() const noexcept
        {
            return rend();
        }

        constexpr size_t size() const noexcept
        {
            return m_size;
        }

        constexpr size_t max_size() const noexcept
        {
            return N;
        }

        constexpr bool empty() const noexcept
        {
            return m_size == 0;
        }

        constexpr T& at(size_t p)
        {
            if (p >= m_size) {
                throw std::out_of_range("static_vector::at");
            }
            return m_data.at(p);
        }

        constexpr const T& at(size_t p) const
        {
            return at(p);
        }

        constexpr T& operator[](size_t p) noexcept
        {
            return m_data[p];
        }

        constexpr const T& operator[](size_t p) const noexcept
        {
            return m_data[p];
        }

        constexpr T& front() noexcept
        {
            return *begin();
        }

        constexpr const T& front() const noexcept
        {
            return *begin();
        }

        constexpr T& back() noexcept
        {
            return *(end() - 1);
        }

        constexpr const T& back() const noexcept
        {
            return *(end() - 1);
        }

        constexpr void push_back(const T& value)
        {
            if (m_size >= N) {
                throw std::out_of_range("static_vector::push_back - capacity exceeded");
            }
            m_data[m_size++] = value;
        }

        constexpr void push_back(T&& value)
        {
            if (m_size >= N) {
                throw std::out_of_range("static_vector::push_back - capacity exceeded");
            }
            m_data[m_size++] = std::move(value);
        }

        constexpr void pop_back()
        {
            if (m_size == 0) {
                throw std::out_of_range("static_vector::pop_back - vector is empty");
            }
            --m_size;
            m_data[m_size] = T{};
        }

        template<typename... Args>
        constexpr T& emplace_back(Args&&... args)
        {
            if (m_size >= N) {
                throw std::out_of_range("static_vector::emplace_back - capacity exceeded");
            }

            m_data[m_size] = T(std::forward<Args>(args)...);
            return m_data[m_size++];
        }

        constexpr void clear() noexcept
        {
            for (size_t i = 0; i < m_size; ++i) {
                m_data[i] = T{};
            }
            m_size = 0;
        }

        constexpr T* data() noexcept
        {
            return m_data.data();
        }

        constexpr const T* data() const noexcept
        {
            return m_data.data();
        }

    private:
        std::array<T, N> m_data;
        size_t           m_size = 0;
    };

} // namespace tavros::core
