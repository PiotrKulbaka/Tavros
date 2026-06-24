#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/fixed_string.hpp>

namespace tavros::core
{

    template<class T>
    class basic_resource
    {
    public:
        using hash_type = size_t;

        static hash_type make_hash(string_view name) noexcept
        {
            return std::hash<string_view>{}(name);
        }

    public:
        basic_resource(string_view name) noexcept
            : m_name(name)
            , m_hash(make_hash(name))
        {
        }

        ~basic_resource() noexcept = default;

        string_view name() const noexcept
        {
            return m_name;
        }

        hash_type hash() const noexcept
        {
            return m_hash;
        }

    private:
        short_string m_name;
        hash_type    m_hash;
    };


    template<class T>
    class basic_resource_ref
    {
    public:
        basic_resource_ref() noexcept = default;

        basic_resource_ref(T* res) noexcept
            : m_resource(res)
        {
        }

        explicit operator bool() const noexcept
        {
            return m_resource != nullptr;
        }

        string_view name() const noexcept
        {
            return m_resource ? m_resource->name() : "";
        }

        basic_resource<T>::hash_type hash() const noexcept
        {
            return m_resource ? m_resource->hash() : 0;
        }

        const T* operator->() const noexcept
        {
            TAV_ASSERT(m_resource);
            return m_resource;
        }

        T* operator->() noexcept
        {
            TAV_ASSERT(m_resource);
            return m_resource;
        }

        const T& operator*() const noexcept
        {
            TAV_ASSERT(m_resource);
            return *m_resource;
        }

        T& operator*() noexcept
        {
            TAV_ASSERT(m_resource);
            return *m_resource;
        }

    private:
        T* m_resource = nullptr;
    };


    enum class resource_status : uint8
    {
        unloaded,
        pending,
        ready,
        failed,
    };

} // namespace tavros::core
