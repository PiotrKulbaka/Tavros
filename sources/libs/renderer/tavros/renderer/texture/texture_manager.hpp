#pragma once

#include <tavros/core/types.hpp>
#include <tavros/core/ids/handle_base.hpp>
#include <tavros/core/resource/resource_registry.hpp>
#include <tavros/renderer/rhi/command_queue.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/gpu_stage_buffer.hpp>
#include <tavros/assets/image/image.hpp>
#include <tavros/assets/image/image_view.hpp>
#include <tavros/assets/asset_manager.hpp>

namespace tavros::renderer
{

    struct texture_tag : tavros::core::handle_type_registration<0x2001>
    {
    };
    using texture_handle = tavros::core::handle_base<texture_tag>;

    struct gpu_texture_view : core::resource_base
    {
        rhi::texture_handle gpu_handle;
        rhi::texture_type   type = rhi::texture_type::texture_2d;
        rhi::pixel_format   format = rhi::pixel_format::none;
        uint32              width = 0;
        uint32              height = 0;
        uint32              depth = 1;
    };

    class texture_manager
    {
    public:
        using registry_type = core::resource_registry<gpu_texture_view, texture_tag>;
        using const_iterator = registry_type::const_iterator;

        explicit texture_manager(core::shared_ptr<assets::asset_manager> am) noexcept;

        ~texture_manager() noexcept = default;

        texture_handle load_cube(
            core::string_view path_px,
            core::string_view path_nx,
            core::string_view path_py,
            core::string_view path_ny,
            core::string_view path_pz,
            core::string_view path_nz,
            bool              gen_mipmaps = true,
            rhi::pixel_format pf = rhi::pixel_format::none
        );

        texture_handle load(
            assets::image     im,
            core::string_view key,
            bool              gen_mipmaps = true,
            rhi::texture_type tt = rhi::texture_type::texture_2d,
            rhi::pixel_format pf = rhi::pixel_format::none
        );

        texture_handle load(
            core::string_view path,
            bool              gen_mipmaps = true,
            rhi::texture_type tt = rhi::texture_type::texture_2d,
            rhi::pixel_format pf = rhi::pixel_format::none
        );

        void acquire(texture_handle handle) noexcept;

        void release(texture_handle handle);

        [[nodiscard]] const gpu_texture_view* get(texture_handle handle) const noexcept;

        [[nodiscard]] rhi::texture_handle get_gpu_handle(texture_handle handle) const noexcept;

        [[nodiscard]] bool has_pending() const noexcept;

        void flush_pending(rhi::graphics_device& device, gpu_stage_buffer& stage, rhi::command_queue& cmd);

        void clear();

        /** @brief Returns const iterator to the beginning. */
        [[nodiscard]] const_iterator begin() const noexcept
        {
            return m_registry.begin();
        }

        /** @brief Returns const iterator to the beginning. */
        [[nodiscard]] const_iterator cbegin() const noexcept
        {
            return m_registry.cbegin();
        }

        /** @brief Returns const iterator to the end. */
        [[nodiscard]] const_iterator end() const noexcept
        {
            return m_registry.end();
        }

        /** @brief Returns const iterator to the end. */
        [[nodiscard]] const_iterator cend() const noexcept
        {
            return m_registry.cend();
        }

    private:
        core::shared_ptr<assets::asset_manager> m_am;
        registry_type                           m_registry;

        struct pending_load
        {
            texture_handle                       handle;
            core::fixed_vector<assets::image, 6> images;
            bool                                 gen_mipmaps = true;
            bool                                 use_srgb = true;
            core::string                         path;
        };
        core::vector<pending_load>        m_pending;
        core::vector<rhi::texture_handle> m_deferred_destroy;
    };

} // namespace tavros::renderer
