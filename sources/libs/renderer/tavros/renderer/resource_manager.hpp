#pragma once

#include <tavros/renderer/texture/texture_manager.hpp>
#include <tavros/renderer/texture/sampler_library.hpp>
#include <tavros/renderer/render_target/render_target_manager.hpp>
#include <tavros/renderer/material/material_manager.hpp>
#include <tavros/renderer/upload_context.hpp>

namespace tavros::renderer
{

    class resource_manager : core::noncopyable, core::nonmovable
    {
    public:
        resource_manager(rhi::graphics_device* gdevice, core::shared_ptr<assets::asset_manager> am, core::shared_ptr<tef::workspace> ws) noexcept;

        ~resource_manager() noexcept;

        void begin_frame() noexcept;
        void end_frame() noexcept;

        template<class Res>
        core::basic_resource_ref<Res> load(core::string_view name, core::string_view params = "depth32f")
        {
            if constexpr (std::is_same_v<Res, texture>) {
                return m_tex_mgr.load(m_upctx, name, *m_ws);
            } else if constexpr (std::is_same_v<Res, render_target>) {
                return m_rt_mgr.load(name, *m_ws);
            } else if constexpr (std::is_same_v<Res, material>) {
                auto ds = rhi::pixel_format::none;
                if (params == "depth32f") {
                    ds = rhi::pixel_format::depth32f;
                } else if (params == "stencil8") {
                    ds = rhi::pixel_format::stencil8;
                } else if (params == "depth24_stencil8") {
                    ds = rhi::pixel_format::depth24_stencil8;
                } else if (params == "depth32f_stencil8") {
                    ds = rhi::pixel_format::depth32f_stencil8;
                }
                return m_mt_mgr.load(name, *m_ws, 1, ds);
            } else {
                static_assert(sizeof(Res) == 0, "load not implemented for this resourece type");
            }
        }

        template<class Res>
        void release(core::basic_resource_ref<Res>& ref) noexcept
        {
            if constexpr (std::is_same_v<Res, texture>) {
                m_tex_mgr.release(ref);
                ref = {};
            } else if constexpr (std::is_same_v<Res, render_target>) {
                m_rt_mgr.release(ref);
                ref = {};
            } else if constexpr (std::is_same_v<Res, material>) {
                m_mt_mgr.release(ref);
                ref = {};
            } else {
                static_assert(sizeof(Res) == 0, "release not implemented for this resourece type");
            }
        }

        texture_manager& textures() noexcept
        {
            return m_tex_mgr;
        }

        const texture_manager& textures() const noexcept
        {
            return m_tex_mgr;
        }

        render_target_manager& render_targets() noexcept
        {
            return m_rt_mgr;
        }

        const render_target_manager& render_targets() const noexcept
        {
            return m_rt_mgr;
        }

        material_manager& materials() noexcept
        {
            return m_mt_mgr;
        }

        const material_manager& materials() const noexcept
        {
            return m_mt_mgr;
        }

        const sampler_library& samplers() const noexcept
        {
            return m_smp_lib;
        }

    private:
        rhi::graphics_device*                   m_gdevice = nullptr;
        core::shared_ptr<assets::asset_manager> m_am;
        core::shared_ptr<tef::workspace>        m_ws;

        upload_context        m_upctx;
        texture_manager       m_tex_mgr;
        render_target_manager m_rt_mgr;
        material_manager      m_mt_mgr;
        sampler_library       m_smp_lib;
    };

} // namespace tavros::renderer

