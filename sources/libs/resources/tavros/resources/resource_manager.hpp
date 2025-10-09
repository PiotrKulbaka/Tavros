#pragma once

#include <tavros/core/memory/memory.hpp>
#include <tavros/core/noncopyable.hpp>
#include <tavros/core/memory/resizable_buffer.hpp>

#include <tavros/resources/resource_provider.hpp>

#include <tavros/core/containers/vector.hpp>

namespace tavros::resources
{

    class resource_manager : core::noncopyable
    {
    public:
        resource_manager();

        ~resource_manager();

        void mount(core::shared_ptr<resource_provider> provider);

        bool exists(core::string_view path) const;

        core::shared_ptr<resource> open(core::string_view path);

        size_t read_data(core::string_view path, core::resizable_buffer<uint8>& buffer);

    private:
        core::vector<core::shared_ptr<resource_provider>> m_providers;
    };

} // namespace tavros::resources
