#pragma once

#include <tavros/core/memory/memory.hpp>
#include <tavros/core/memory/mallocator.hpp>
#include <tavros/core/noncopyable.hpp>

#include <tavros/resources/resource_provider.hpp>
#include <tavros/resources/resource_access.hpp>

#include <tavros/core/containers/static_vector.hpp>

namespace tavros::resources
{

    class resource_manager : core::noncopyable
    {
    public:
        /**
         * There should be a limited number of providers. The more providers there are,
         * the longer it will take to find a resource.
         * If needed many providers, then need to change the resource search algorithm.
         */
        constexpr static size_t k_max_resource_providers = 32;

    public:
        resource_manager();

        ~resource_manager();

        template<typename T, typename... Args>
            requires std::derived_from<T, resource_provider>
        void mount(Args&&... args)
        {
            auto provider = core::make_shared<T>(std::forward<Args>(args)...);
            m_providers.push_back(provider);
        }

        bool exists(core::string_view path) const;

        core::shared_ptr<resource> open(core::string_view path, resource_access access = resource_access::read_only);

    private:
        using providers_vector = core::static_vector<core::shared_ptr<resource_provider>, k_max_resource_providers>;
        core::mallocator m_alc;
        providers_vector m_providers;
    };

} // namespace tavros::resources
