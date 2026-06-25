#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/fixed_string.hpp>

namespace tavros::core
{

    /**
     * @brief Base class for all resources managed by resource managers.
     *
     * Provides a common interface for resource identification by name and hash.
     * Resources are initially created in an invalid state and become valid only
     * after successful initialization.
     *
     * Typical usage:
     * - Construct a resource object.
     * - Perform resource-specific initialization.
     * - Call set_valid() if initialization succeeds.
     * - Resource managers store only valid resources.
     *
     * @tparam T Derived resource type.
     */
    template<class T>
    class basic_resource
    {
    public:
        /// Hash type used for resource identification.
        using hash_type = size_t;

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
        /**
         * @brief Constructs a resource with the specified name.
         *
         * Newly constructed resources are invalid until set_valid() is called.
         *
         * @param name Resource name.
         */
        basic_resource(string_view name) noexcept
            : m_name(name)
            , m_hash(0)
        {
        }

        /** @brief Move constructor. */
        basic_resource(basic_resource&& other) noexcept
            : m_name(other.m_name)
            , m_hash(other.m_hash)
        {
            other.m_name.clear();
            other.m_hash = 0;
        }

        /** Default destructor. */
        ~basic_resource() noexcept = default;

        /** @brief Returns the resource name. */
        string_view name() const noexcept
        {
            return m_name;
        }

        /** @brief Returns the resource hash. */
        hash_type hash() const noexcept
        {
            return m_hash;
        }

        /**
         * @brief Checks whether the resource was successfully initialized.
         *
         * A resource is considered valid only after set_valid() has been called.
         * Resource managers may use this flag to determine whether resource
         * creation completed successfully.
         *
         * @return True if the resource is valid, otherwise false.
         */
        bool is_valid() const noexcept
        {
            return m_hash != 0;
        }

    protected:
        /**
         * @brief Marks the resource as successfully initialized.
         *
         * Computes and stores the resource hash, transitioning the resource
         * into a valid state.
         */
        void set_valid() noexcept
        {
            m_hash = make_hash(m_name);
        }

    private:
        short_string m_name;
        hash_type    m_hash;
    };


    /**
     * @brief Non-owning reference to a resource.
     *
     * Provides convenient pointer-like access to a resource without affecting
     * its lifetime. The reference may be null.
     *
     * @tparam T Resource type.
     */
    template<class T>
    class basic_resource_ref
    {
    public:
        /** Constructs an empty reference. */
        basic_resource_ref() noexcept = default;

        /**
         * @brief Constructs a reference from a resource pointer.
         *
         * @param res Resource pointer.
         */
        basic_resource_ref(T* res) noexcept
            : m_resource(res)
        {
        }

        /** @brief Checks whether the reference points to a resource. */
        explicit operator bool() const noexcept
        {
            return m_resource != nullptr;
        }

        /** @brief Returns the referenced resource name. */
        string_view name() const noexcept
        {
            return m_resource ? m_resource->name() : "";
        }

        /** @brief Returns the referenced resource hash. */
        basic_resource<T>::hash_type hash() const noexcept
        {
            return m_resource ? m_resource->hash() : 0;
        }

        /** @brief Returns a pointer to the referenced resource. */
        const T* operator->() const noexcept
        {
            TAV_ASSERT(m_resource);
            return m_resource;
        }

        /** @brief Returns a pointer to the referenced resource. */
        T* operator->() noexcept
        {
            TAV_ASSERT(m_resource);
            return m_resource;
        }

        /** @brief Returns a reference to the resource. */
        const T& operator*() const noexcept
        {
            TAV_ASSERT(m_resource);
            return *m_resource;
        }

        /** @brief Returns a reference to the resource. */
        T& operator*() noexcept
        {
            TAV_ASSERT(m_resource);
            return *m_resource;
        }

    private:
        T* m_resource = nullptr;
    };

    /*enum class resource_status : uint8
    {
        unloaded,
        pending,
        ready,
        failed,
    };*/

} // namespace tavros::core
