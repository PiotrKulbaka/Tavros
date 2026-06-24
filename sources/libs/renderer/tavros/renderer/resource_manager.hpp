#pragma once

#include <tavros/renderer/texture/texture_manager.hpp>
#include <tavros/renderer/render_target/render_target_manager.hpp>
#include <tavros/renderer/shaders/shader_loader.hpp>
#include <tavros/renderer/upload_context.hpp>

namespace tavros::renderer
{

    class resource_manager : core::noncopyable, core::nonmovable
    {
    public:
        resource_manager(rhi::graphics_device* gdevice, core::shared_ptr<assets::asset_manager> am, core::shared_ptr<tef::workspace> ws) noexcept;

        ~resource_manager() noexcept;

        void init(upload_context* upctx) noexcept;
        void shutdown() noexcept;

        void begin_frame() noexcept;
        void end_frame() noexcept;

        template<class Res>
        core::basic_resource_ref<Res> load(core::string_view name)
        {
            TAV_ASSERT(m_upctx);
            if constexpr (std::is_same_v<Res, texture_t>) {
                return m_tex_mgr.load(*m_upctx, name, *m_ws);
            } else if constexpr (std::is_same_v<Res, render_target>) {
                return m_rt_mgr.load(name, *m_ws);
            } else {
                static_assert(sizeof(Res) == 0, "load not implemented for this resourece type");
            }
        }

        template<class Res>
        void release(core::basic_resource_ref<Res> ref) noexcept
        {
            if constexpr (std::is_same_v<Res, texture_t>) {
                return m_tex_mgr.release(ref);
            } else if constexpr (std::is_same_v<Res, render_target>) {
                return m_rt_mgr.release(ref);
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

    private:
        rhi::graphics_device*                   m_gdevice = nullptr;
        core::shared_ptr<assets::asset_manager> m_am;
        core::shared_ptr<tef::workspace>        m_ws;

        upload_context*       m_upctx = nullptr;
        texture_manager       m_tex_mgr;
        render_target_manager m_rt_mgr;
    };

} // namespace tavros::renderer

