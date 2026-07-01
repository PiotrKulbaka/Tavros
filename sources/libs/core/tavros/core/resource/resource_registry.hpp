#pragma once


#include <tavros/core/containers/vector.hpp>
#include <tavros/core/containers/unordered_map.hpp>
#include <tavros/core/ref_counted.hpp>
#include <tavros/core/memory/memory.hpp>
#include <tavros/core/fixed_string.hpp>
#include <tavros/core/resource/resource.hpp>

namespace tavros::core
{

    template<class T>
    class resource_registry
    {
    public:
        using resource_type = T;

        /// Hash type used for resource identification.
        using hash_type = size_t;

        using ref_type = resource_ref<resource_type>;

        struct entry_type
        {
            unique_ptr<resource_type> owner;         // nullptr until status != ready
            resource_type*            ptr = nullptr; // Points to placeholder first
            single_thread_ref_count   rc;
            hash_type                 hash;
            resource_status           status = resource_status::unloaded;
            short_string              name;
        };

        using storage_type = unordered_map<hash_type, entry_type>;
        using iterator = storage_type::iterator;
        using const_iterator = storage_type::const_iterator;

        /**
         * @brief Calculates a hash value for a resource name.
         *
         * @param name Resource name.
         * @return Hash value for the specified name.
         */
        static hash_type make_hash(string_view name) noexcept
        {
            return std::hash<string_view>{}(name);
        }

    public:
        resource_registry() noexcept = default;

        ~resource_registry() noexcept = default;

        void set_placeholder(resource_type* placeholder) noexcept
        {
            m_placeholder = placeholder;
        }

        /**
         * @brief Reserves a slot for a resource, or returns an existing one.
         *
         * This is the ONLY operation that may be called concurrently from
         * multiple threads. It never touches actual resource data — it only
         * registers intent. The resource starts in `unloaded` state and has
         * no data until publish() is called.
         *
         * @return Reference to the (possibly not-yet-loaded) resource slot.
         */
        [[nodiscard]] std::pair<ref_type, bool> make_slot(string_view name)
        {
            const auto h = make_hash(name);
            auto       it = m_resources.find(h);
            if (it != m_resources.end()) {
                it->second.rc.increment();
                return {ref_type{&it->second}, false};
            }

            auto [inserted_it, inserted] = m_resources.emplace(
                h,
                entry_type{
                    .owner = nullptr,
                    .ptr = m_placeholder,
                    .rc = {},
                    .hash = h,
                    .status = resource_status::unloaded,
                    .name = short_string(name)
                }
            );
            TAV_ASSERT(inserted);
            return {ref_type{&inserted_it->second}, true};
        }

        /**
         * @brief The single synchronization point.
         *
         * Must be called from one thread only (e.g. main thread, once per
         * frame). Applies all pending state transitions: publishes loaded
         * data, fires the optional status hook, and destroys resources whose
         * ref count reached zero.
         */
        void sync()
        {
            for (auto& t : m_pending_transitions) {
                auto it = m_resources.find(t.ref.hash());
                if (it == m_resources.end()) {
                    continue; // could've been erased already
                }

                entry_type& entry = it->second;

                switch (t.kind) {
                case transition_kind::set_ready:
                    entry.owner = std::move(t.data);
                    entry.ptr = entry.owner.get();
                    entry.status = resource_status::ready;
                    break;

                case transition_kind::set_failed:
                    entry.status = resource_status::failed;
                    break;

                case transition_kind::acquire:
                    entry.rc.increment();
                    break;

                case transition_kind::release:
                    if (entry.rc.decrement()) {
                        m_resources.erase(it);
                    }
                    break;
                }
            }
            m_pending_transitions.clear();
        }

        /**
         * @brief Publishes loaded resource data.
         *
         * Called by whatever code finished loading (any thread). Data is NOT
         * visible to consumers until sync() runs on the owning thread.
         */
        void publish(ref_type ref, unique_ptr<resource_type> loaded)
        {
            m_pending_transitions.push_back(pending_transition{ref, transition_kind::set_ready, std::move(loaded)});
        }

        /**
         * @brief Marks a resource load as failed.
         */
        void publish_failed(ref_type ref)
        {
            m_pending_transitions.push_back(pending_transition{ref, transition_kind::set_failed, nullptr});
        }

        /**
         * @brief Increments reference count.
         *
         * No-op if the handle is invalid or stale.
         */
        void acquire(ref_type ref) noexcept
        {
            m_pending_transitions.push_back(pending_transition{ref, transition_kind::acquire, nullptr});
        }

        /**
         * @brief Decrements reference count.
         *
         * Does NOT destroy anything immediately — actual destruction happens
         * during sync(). Safe to call from any thread.
         */
        void release(ref_type ref) noexcept
        {
            m_pending_transitions.push_back(pending_transition{ref, transition_kind::release, nullptr});
        }

        /**
         * @brief Finds a mutable resource by handle.
         *
         * @return Pointer to resource, or nullptr if handle is invalid or stale.
         */
        [[nodiscard]] ref_type find(string_view name) noexcept
        {
            const auto h = make_hash(name);
            auto       it = m_resources.find(h);
            if (it != m_resources.end()) {
                return ref_type(&it->second);
            }
            return {};
        }

        /**
         * @return true if a resource with the given key is registered.
         */
        [[nodiscard]] bool contains(string_view name) const noexcept
        {
            const auto h = make_hash(name);
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
        enum class transition_kind : uint8
        {
            set_ready,
            set_failed,
            acquire,
            release,
        };

        struct pending_transition
        {
            ref_type                  ref;
            transition_kind           kind;
            unique_ptr<resource_type> data; // only used for set_ready
        };

    private:
        resource_type* m_placeholder = nullptr;
        storage_type   m_resources;

        core::vector<pending_transition> m_pending_transitions;
    };

} // namespace tavros::core
