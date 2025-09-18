#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/containers/unordered_map.hpp>

#include <atomic>

namespace tavros::core
{

    template<class T, class H = uint32>
    class resource_pool
    {
    public:
        using handle_t = H;

    public:
        resource_pool() = default;
        ~resource_pool() = default;

        constexpr auto begin() noexcept
        {
            return m_storage.begin();
        }

        constexpr auto begin() const noexcept
        {
            return m_storage.begin();
        }

        constexpr auto end() noexcept
        {
            return m_storage.end();
        }

        constexpr auto end() const noexcept
        {
            return m_storage.end();
        }

        constexpr auto cbegin() const noexcept
        {
            return m_storage.cbegin();
        }

        constexpr auto cend() const noexcept
        {
            return m_storage.cend();
        }

        constexpr auto size() const noexcept
        {
            return m_storage.size();
        }

        T* try_get(handle_t handle)
        {
            if (m_last_handle == handle) {
                return m_last;
            }
            auto it = m_storage.find(handle);
            if (it != m_storage.end()) {
                m_last_handle = handle;
                m_last = &it->second;
                return m_last;
            }
            return nullptr;
        }

        const T* try_get(handle_t handle) const
        {
            return try_get(handle);
        }

        [[nodiscard]] handle_t insert(T&& desc)
        {
            auto handle = m_counter.fetch_add(1);
            m_storage[handle] = std::move(desc);
            reset_last_cache();
            return handle;
        }

        template<typename... Args>
        [[nodiscard]] handle_t emplace(Args&&... args)
        {
            auto handle = m_counter.fetch_add(1);
            m_storage.emplace(std::piecewise_construct, std::forward_as_tuple(handle), std::forward_as_tuple(std::forward<Args>(args)...));
            reset_last_cache();
            return handle;
        }

        void remove(handle_t handle)
        {
            m_storage.erase(handle);
            reset_last_cache();
        }

        void clear()
        {
            m_storage.clear();
            reset_last_cache();
        }

    private:
        void reset_last_cache()
        {
            m_last_handle = 0;
            m_last = nullptr;
        }

    private:
        std::atomic<handle_t>            m_counter = 1;
        core::unordered_map<handle_t, T> m_storage;
        handle_t                         m_last_handle = 0;
        T*                               m_last = nullptr;
    };

} // namespace tavros::core
