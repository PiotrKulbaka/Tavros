#pragma once

#include <tavros/core/memory/memory.hpp>
#include <tavros/core/memory/mallocator.hpp>
#include <tavros/core/noncopyable.hpp>

#include <tavros/resources/resource_provider.hpp>

#include <tavros/core/containers/vector.hpp>

namespace tavros::resources
{

    class resource_manager : core::noncopyable
    {
    public:
        resource_manager();

        ~resource_manager();

        template<typename T, typename... Args>
            requires std::derived_from<T, resource_provider>
        void mount(Args&&... args)
        {
            auto provider = core::make_shared<T>(std::forward<Args>(args)...);
            for (auto& p : m_providers) {
                if (p == provider) {
                    return;
                }
            }
            m_providers.push_back(provider);
        }

        bool exists(core::string_view path) const;

        core::shared_ptr<resource> open(core::string_view path);

    private:
        core::mallocator                                  m_alc;
        core::vector<core::shared_ptr<resource_provider>> m_providers;
    };

} // namespace tavros::resources
