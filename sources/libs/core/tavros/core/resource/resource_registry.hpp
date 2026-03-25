#pragma once

#include <tavros/core/noncopyable.hpp>
#include <tavros/core/string_view.hpp>
#include <tavros/core/string.hpp>
#include <tavros/core/ref_counted.hpp>
#include <tavros/core/containers/unordered_map.hpp>
#include <tavros/core/utils/string_hash.hpp>
#include <tavros/core/resource/object_pool.hpp>

namespace tavros::core
{

    /**
     * @brief Resource registry with handle-based access and reference counting.
     *
     * Stores resources in an @ref object_pool and associates them with string keys.
     * Provides:
     * - O(1) handle-based lookup via @ref find
     * - key-based deduplication on @ref insert
     * - manual lifetime control via reference counting
     *
     * Handles include a generation counter, so stale handles are detected safely.
     *
     * @tparam Resource Resource type.
     * @tparam Tag      Handle tag type.
     */
    template<class Resource, handle_tagged Tag>
    class resource_registry : noncopyable
    {
    public:
        using resource_type = Resource;
        using handle_type = handle_base<Tag>;
        using ref_count = single_thread_ref_count;

        /**
         * @brief Internal entry stored in the pool.
         */
        struct entry
        {
            /// Resource value.
            resource_type res;

            /// Cache key - used for erase and diagnostics.
            string key;

            /// Reference counter.
            ref_count rc;
        };

        using pool_type = object_pool<entry, Tag>;

        using const_iterator = pool_type::const_iterator;

    public:
        resource_registry() noexcept = default;

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
        [[nodiscard]] handle_type insert(string_view key, resource_type res)
        {
            auto it = m_key_to_h.find(key);
            if (it != m_key_to_h.end()) {
                auto* e = m_pool.find(it->second);
                if (e) {
                    e->rc.increment();
                    return it->second;
                }
            }

            auto handle = m_pool.emplace(entry{std::move(res), string{key}, ref_count{}});
            if (!handle) {
                return handle_type{};
            }
            m_key_to_h.emplace(string{key}, handle);

            return handle;
        }

        /**
         * @brief Increments reference count.
         *
         * No-op if the handle is invalid or stale.
         */
        void acquire(handle_type handle)
        {
            if (auto* e = m_pool.find(handle)) {
                e->rc.increment();
            }
        }

        /**
         * @brief Decrements reference count.
         *
         * @param handle Resource handle.
         * @return @c true if the count reached zero.
         *
         * @note When this returns true, caller must:
         *       1. destroy resource data (unload)
         *       2. call @ref erase
         */
        [[nodiscard]] bool release(handle_type handle)
        {
            auto* e = m_pool.find(handle);
            return e ? e->rc.decrement() : false;
        }

        /**
         * @brief Removes an entry from the registry.
         *
         * Invalidates the handle and erases the associated key.
         *
         * @note Must be called only after @ref release returned true.
         */
        void erase(handle_type handle)
        {
            auto* e = m_pool.find(handle);
            if (!e) {
                return;
            }
            m_key_to_h.erase(e->key);
            m_pool.erase(handle);
        }

        /**
         * @brief Finds a handle by key.
         *
         * Does not modify reference count.
         *
         * @return Handle or invalid handle if not found.
         */
        [[nodiscard]] handle_type find_handle(string_view key) const noexcept
        {
            auto it = m_key_to_h.find(key);
            return it != m_key_to_h.end() ? it->second : handle_type{};
        }

        /**
         * @brief Finds a resource by handle.
         *
         * @return Pointer to resource, or nullptr if handle is invalid or stale.
         */
        [[nodiscard]] const resource_type* find(handle_type handle) const noexcept
        {
            const auto* e = m_pool.find(handle);
            return e ? &e->res : nullptr;
        }

        /**
         * @brief Finds a mutable resource by handle.
         *
         * @return Pointer to resource, or nullptr if handle is invalid or stale.
         */
        [[nodiscard]] resource_type* find(handle_type handle) noexcept
        {
            auto* e = m_pool.find(handle);
            return e ? &e->res : nullptr;
        }

        /**
         * @return true if a resource with the given key is registered.
         */
        [[nodiscard]] bool contains(string_view key) const noexcept
        {
            return m_key_to_h.contains(key);
        }

        /**
         * @return true if the handle refers to a live entry.
         */
        [[nodiscard]] bool contains(handle_type handle) const noexcept
        {
            return m_pool.contains(handle);
        }

        /**
         * @brief Returns number of stored entries.
         */
        [[nodiscard]] size_t size() const noexcept
        {
            return m_pool.size();
        }

        /**
         * @brief Checks if registry is empty.
         */
        [[nodiscard]] bool empty() const noexcept
        {
            return m_pool.empty();
        }

        /**
         * @brief Returns current storage capacity.
         */
        [[nodiscard]] size_t capacity() const noexcept
        {
            return m_pool.capacity();
        }

        /**
         * @brief Reserves storage for at least @p count entries.
         */
        void reserve(size_t count)
        {
            m_pool.reserve(count);
            m_key_to_h.reserve(count);
        }

        /**
         * @brief Clears all entries.
         *
         * All handles become invalid. Capacity is preserved.
         */
        void clear()
        {
            m_pool.clear();
            m_key_to_h.clear();
        }

        /** @brief Returns const iterator to the beginning. */
        [[nodiscard]] const_iterator begin() const noexcept
        {
            return m_pool.begin();
        }

        /** @brief Returns const iterator to the beginning. */
        [[nodiscard]] const_iterator cbegin() const noexcept
        {
            return m_pool.cbegin();
        }

        /** @brief Returns const iterator to the end. */
        [[nodiscard]] const_iterator end() const noexcept
        {
            return m_pool.end();
        }

        /** @brief Returns const iterator to the end. */
        [[nodiscard]] const_iterator cend() const noexcept
        {
            return m_pool.cend();
        }

    private:
        using key_map = unordered_map<string, handle_type, string_hash, string_equal>;

        pool_type m_pool;
        key_map   m_key_to_h;
    };

} // namespace tavros::core
