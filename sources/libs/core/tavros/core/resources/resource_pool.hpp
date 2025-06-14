#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/containers/unordered_map.hpp>

#include <atomic>

namespace tavros::core
{

    template<class T>
    class resource_pool
    {
    public:
        using handle_t = uint32;

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
            auto it = m_storage.find(handle);
            if (it != m_storage.end()) {
                return &it->second;
            }
            return nullptr;
        }

        const T* try_get(handle_t handle) const
        {
            return try_get(handle);
        }

        [[nodiscard]] handle_t insert(const T& desc)
        {
            auto handle = m_counter.fetch_add(1);
            m_storage[handle] = desc;
            return handle;
        }

        void remove(handle_t handle)
        {
            m_storage.erase(handle);
        }

        void clear()
        {
            m_storage.clear();
        }

    private:
        std::atomic<handle_t>          m_counter = 1;
        core::unordered_map<uint32, T> m_storage;
    };

} // namespace tavros::core
