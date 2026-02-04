#pragma once

#include "source_file.hpp"

#include <tavros/core/noncopyable.hpp>
#include <tavros/core/nonmovable.hpp>
#include <tavros/core/utils/string_string_view_comparator.hpp>
#include <tavros/core/memory/memory.hpp>
#include <tavros/core/containers/vector.hpp>
#include <tavros/core/containers/map.hpp>
#include <tavros/resources/resource_manager.hpp>

namespace tavros::renderer
{

    class shader_preprocessor : core::noncopyable
    {
    public:
        shader_preprocessor(core::shared_ptr<resources::resource_manager> rm) noexcept;
        ~shader_preprocessor() noexcept = default;

        core::string load_shader_source(core::string_view path);

    private:
        void load_r(core::string_view path);
        core::string load_file(core::string_view path);

    private:
        std::map<core::string, source_file, core::string_string_view_comparator> m_files;
        core::shared_ptr<resources::resource_manager> m_resource_manager;
    };

} // namespace tavros::renderer
