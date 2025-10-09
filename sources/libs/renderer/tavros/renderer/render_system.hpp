#pragma once

#include <tavros/core/noncopyable.hpp>
#include <tavros/core/nonmovable.hpp>
#include <tavros/core/memory/mallocator.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/texture.hpp>
#include <tavros/renderer/render_target.hpp>

namespace tavros::renderer
{
    class render_system : core::noncopyable, core::nonmovable // TODO: remove nonmovable
    {
    public:
        render_system(rhi::graphics_device* device); // TODO: remove graphics device dependency, for now it's required because it's development
        ~render_system();

        texture_view create_texture(const texture_create_info& info);

        void release_texture(texture_view tex);

        render_target_view create_render_target(const render_target_create_info& info);

        void release_render_target(render_target_view rt);

    private:
        rhi::graphics_device*            m_gdevice;
        core::mallocator                 m_mallocator;
        core::object_pool<texture>       m_textures;
        core::object_pool<render_target> m_render_targets;
    };

} // namespace tavros::renderer
