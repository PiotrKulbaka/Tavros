#pragma once

#include <tavros/core/containers/unordered_map.hpp>
#include <tavros/core/resource/resource.hpp>
#include <tavros/core/string.hpp>
#include <tavros/core/ref_counted.hpp>

namespace tavros::core
{

    template<class T>
        requires std::is_base_of_v<basic_resource<T>, T>
    class basic_resource_manager
    {
    public:
        using resource_type = T;
        using resource_ref_type = basic_resource_ref<resource_type>;

        /**
         * @brief Internal entry stored in the pool.
         */
        struct entry_type
        {
            resource_type           res; ///< Resource value.
            single_thread_ref_count rc;  ///< Reference counter.
        };

        using hash_type = typename resource_type::hash_type;
        using storage_type = unordered_map<hash_type, entry_type>;
        using iterator = storage_type::iterator;
        using const_iterator = storage_type::const_iterator;

    public:
        ~basic_resource_manager() noexcept = default;

        /**
         * @brief Inserts a resource or acquires an existing one.
         *
         * If @p key already exists, increments its reference count and returns the existing handle.
         * Otherwise inserts a new entry with ref_count = 1.
         *
         * @param key Unique resource key.
         * @param res Resource value (moved into storage).
         *
         * @return Valid handle on success, invalid handle if insertion failed.
         */
        [[nodiscard]] resource_ref_type insert(resource_type res)
        {
            const auto h = res.hash();
            TAV_ASSERT(h != 0);
            if (auto it = m_resources.find(h); it != m_resources.end()) {
                TAV_ASSERT(it->second.res.name() == res.name());
                it->second.rc.increment();
                return resource_ref_type(&it->second.res);
            }

            auto [it, inserted] = m_resources.emplace(h, entry_type{.res = std::move(res), .rc = {}});

            return resource_ref_type(inserted ? &it->second.res : nullptr);
        }

        /**
         * @brief Increments reference count.
         *
         * No-op if the handle is invalid or stale.
         */
        void acquire(resource_ref_type ref) noexcept
        {
            const auto h = ref->hash();
            if (auto it = m_resources.find(h); it != m_resources.end()) {
                it->second.rc.increment();
            }
        }

        /**
         * @brief Decrements reference count and release resource.
         *
         * @param handle Resource handle.
         * @return @c true if the count reached zero.
         */
        [[nodiscard]] bool release(resource_ref_type ref) noexcept
        {
            const auto h = ref->hash();
            if (auto it = m_resources.find(h); it != m_resources.end()) {
                if (it->second.rc.decrement()) {
                    m_resources.erase(it);
                    return true;
                }
                return false;
            }

            return false;
        }

        /**
         * @brief Removes an entry from the registry.
         *
         * Invalidates the handle and erases the associated key.
         */
        void erase(resource_ref_type ref) noexcept
        {
            const auto h = ref->hash();
            if (auto it = m_resources.find(h); it != m_resources.end()) {
                m_resources.erase(it);
            }
        }

        /**
         * @brief Finds a mutable resource by handle.
         *
         * @return Pointer to resource, or nullptr if handle is invalid or stale.
         */
        [[nodiscard]] resource_ref_type find(string_view name) noexcept
        {
            const auto h = basic_resource<resource_type>::make_hash(name);
            auto       it = m_resources.find(h);
            return resource_ref_type(it != m_resources.end() ? &it->second.res : nullptr);
        }

        /**
         * @return true if a resource with the given key is registered.
         */
        [[nodiscard]] bool contains(string_view name) const noexcept
        {
            const auto h = basic_resource<resource_type>::make_hash(name);
            return m_resources.contains(h);
        }

        /**
         * @brief Returns number of stored entries.
         */
        [[nodiscard]] size_t size() const noexcept
        {
            return m_resources.size();
        }

        /**
         * @brief Checks if registry is empty.
         */
        [[nodiscard]] bool empty() const noexcept
        {
            return m_resources.empty();
        }

        /**
         * @brief Reserves storage for at least @p count entries.
         */
        void reserve(size_t count)
        {
            m_resources.reserve(count);
        }

        /**
         * @brief Clears all entries.
         *
         * All handles become invalid. Capacity is preserved.
         */
        void clear() noexcept
        {
            m_resources.clear();
        }

        /** @brief Returns const iterator to the beginning. */
        [[nodiscard]] iterator begin() noexcept
        {
            return m_resources.begin();
        }

        /** @brief Returns const iterator to the beginning. */
        [[nodiscard]] const_iterator begin() const noexcept
        {
            return m_resources.begin();
        }

        /** @brief Returns iterator to the end. */
        [[nodiscard]] iterator end() noexcept
        {
            return m_resources.end();
        }

        /** @brief Returns const iterator to the end. */
        [[nodiscard]] const_iterator end() const noexcept
        {
            return m_resources.end();
        }

        /** @brief Returns const iterator to the beginning. */
        [[nodiscard]] const_iterator cbegin() const noexcept
        {
            return m_resources.cbegin();
        }

        /** @brief Returns const iterator to the end. */
        [[nodiscard]] const_iterator cend() const noexcept
        {
            return m_resources.cend();
        }

    private:
        storage_type m_resources;
    };

} // namespace tavros::core
