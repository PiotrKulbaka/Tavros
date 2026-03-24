#pragma once

#include <tavros/core/noncopyable.hpp>
#include <tavros/core/string_view.hpp>
#include <tavros/core/string.hpp>
#include <tavros/core/ref_counted.hpp>
#include <tavros/core/containers/unordered_map.hpp>
#include <tavros/core/utils/string_hash.hpp>
#include <tavros/core/resource/object_pool.hpp>

#include <shared_mutex>

namespace tavros::core
{

    /**
     * @brief Thread-safe resource registry backed by object_pool and ref_counted.
     *
     * Each entry stores a ref_counted<Resource> and its string key inside the pool slot.
     * A secondary map (key -> handle) is used only during load() for deduplication.
     *
     * find(handle) is O(1) via object_pool - safe to call every frame.
     * Handles carry a generation counter, so stale handles are detected automatically.
     *
     * Thread safety:
     *   - find / contains / size - shared_lock, non-blocking for concurrent readers.
     *   - insert / acquire / release / erase / clear - unique_lock.
     *
     * @tparam Resource  Stored resource type.
     * @tparam Tag       Handle tag (e.g. texture_tag, mesh_tag).
     */
    template<class Resource, handle_tagged Tag>
    class resource_registry : noncopyable
    {
    public:
        using resource_type = Resource;
        using handle_type = handle_base<Tag>;

        struct entry
        {
            /// Resource value + reference counter.
            ref_counted<resource_type> rc;

            /// Cache key - used for erase and diagnostics.
            string key;
        };

        using pool_type = object_pool<entry, Tag>;

        resource_registry() noexcept = default;

        /** @return Handle for the given key, or an invalid handle if not found. Does not affect ref_count. */
        [[nodiscard]] handle_type find_handle(string_view key) const noexcept
        {
            std::shared_lock lock(m_mutex);
            auto             it = m_key_to_h.find(key);
            return it != m_key_to_h.end() ? it->second : handle_type{};
        }

        /** @return Pointer to the resource, or nullptr if the handle is invalid or stale. */
        [[nodiscard]] const resource_type* find(handle_type handle) const noexcept
        {
            std::shared_lock lock(m_mutex);
            const auto*      e = m_pool.find(handle);
            return e ? &e->rc.value() : nullptr;
        }

        /** @return Mutable pointer to the resource, or nullptr if the handle is invalid or stale. */
        [[nodiscard]] resource_type* find(handle_type handle) noexcept
        {
            std::shared_lock lock(m_mutex);
            auto*            e = m_pool.find(handle);
            return e ? &e->rc.value() : nullptr;
        }

        /** @return true if a resource with the given key is registered. */
        [[nodiscard]] bool contains(string_view key) const noexcept
        {
            std::shared_lock lock(m_mutex);
            return m_key_to_h.contains(key);
        }

        /** @return true if the handle refers to a live entry. */
        [[nodiscard]] bool contains(handle_type handle) const noexcept
        {
            std::shared_lock lock(m_mutex);
            return m_pool.contains(handle);
        }

        /** @return Number of registered resources. */
        [[nodiscard]] size_t size() const noexcept
        {
            std::shared_lock lock(m_mutex);
            return m_pool.size();
        }

        /** @return true if no resources are registered. */
        [[nodiscard]] bool empty() const noexcept
        {
            std::shared_lock lock(m_mutex);
            return m_pool.empty();
        }

        /** @return Current ref_count for the given handle, or 0 if not found. */
        [[nodiscard]] int32 ref_count(handle_type handle) const noexcept
        {
            std::shared_lock lock(m_mutex);
            const auto*      e = m_pool.find(handle);
            return e ? e->rc.count() : 0;
        }

        /**
         * @brief Register a resource under the given key with ref_count = 1.
         *
         * If the key already exists, increments ref_count and returns the existing handle.
         *
         * @return Valid handle, or invalid handle if the pool is full.
         */
        [[nodiscard]] handle_type insert(string_view key, resource_type res)
        {
            std::unique_lock lock(m_mutex);

            auto it = m_key_to_h.find(key);
            if (it != m_key_to_h.end()) {
                auto* e = m_pool.find(it->second);
                if (e) {
                    e->rc.acquire();
                    return it->second;
                }
            }

            auto handle = m_pool.emplace(entry{ref_counted<resource_type>{std::move(res)}, string{key}});
            if (!handle) {
                return handle_type{};
            }

            // ref_counted starts at 0 - acquire once to represent the first owner
            m_pool.find(handle)->rc.acquire();
            m_key_to_h.emplace(string{key}, handle);
            return handle;
        }

        /** @brief Increment ref_count. No-op if handle is invalid. */
        void acquire(handle_type handle)
        {
            std::unique_lock lock(m_mutex);
            if (auto* e = m_pool.find(handle)) {
                e->rc.acquire();
            }
        }

        /**
         * @brief Decrement ref_count.
         * @return true if the count reached zero - caller must invoke unload_impl() then erase().
         */
        [[nodiscard]] bool release(handle_type handle)
        {
            std::unique_lock lock(m_mutex);
            auto*            e = m_pool.find(handle);
            return e ? e->rc.release() : false;
        }

        /**
         * @brief Remove the entry from the registry.
         *
         * Must be called only after release() returned true and unload_impl() has been invoked.
         */
        void erase(handle_type handle)
        {
            std::unique_lock lock(m_mutex);

            auto* e = m_pool.find(handle);
            if (!e) {
                return;
            }
            m_key_to_h.erase(e->key);
            m_pool.erase(handle);
        }

        /**
         * @brief Forcefully remove all entries without invoking unload_impl.
         *
         * Intended for shutdown only.
         */
        void clear()
        {
            std::unique_lock lock(m_mutex);
            m_pool.clear();
            m_key_to_h.clear();
        }

        /**
         * @brief Direct access to the underlying pool for iteration.
         *
         * Not thread-safe. Call only from the main thread when no loading is in progress.
         *
         * @code
         * for (auto [handle, entry_ptr] : registry.pool()) {
         *     inspect(entry_ptr->rc.value());
         * }
         * @endcode
         */
        [[nodiscard]] const pool_type& pool() const noexcept
        {
            return m_pool;
        }

        [[nodiscard]] pool_type& pool() noexcept
        {
            return m_pool;
        }

    private:
        using key_map = unordered_map<string, handle_type, string_hash, string_equal>;

        mutable std::shared_mutex m_mutex;
        pool_type                 m_pool;
        key_map                   m_key_to_h;
    };

} // namespace tavros::core