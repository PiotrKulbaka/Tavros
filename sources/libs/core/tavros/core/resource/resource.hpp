#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/fixed_string.hpp>

namespace tavros::core
{

    enum class resource_status : uint8
    {
        unloaded,
        ready,
        failed,
    };

    template<class T>
    class resource_registry;

    template<class T>
    class resource_ref
    {
    public:
        resource_ref() noexcept = default;

        explicit operator bool() const noexcept
        {
            return m_entry != nullptr;
        }

        resource_status status() const noexcept
        {
            return m_entry ? m_entry->status : resource_status::unloaded;
        }

        bool is_ready() const noexcept
        {
            return status() == resource_status::ready;
        }

        string_view name() const noexcept
        {
            if (m_entry) {
                return m_entry->name;
            }
            return {};
        }

        size_t hash() const noexcept
        {
            return m_entry ? m_entry->hash : 0;
        }

        const T* get() const noexcept
        {
            return m_entry ? m_entry->ptr : nullptr;
        }

        T* get() noexcept
        {
            return m_entry ? m_entry->ptr : nullptr;
        }

        const T* operator->() const noexcept
        {
            return get();
        }

        T* operator->() noexcept
        {
            return get();
        }

        const T* operator*() const noexcept
        {
            return get();
        }

        T* operator*() noexcept
        {
            return get();
        }

    private:
        template<class U>
        friend class resource_registry;

        resource_ref(resource_registry<T>::entry_type* entry) noexcept
            : m_entry(entry)
        {
        }

        resource_registry<T>::entry_type* m_entry = nullptr;
    };

} // namespace tavros::core
