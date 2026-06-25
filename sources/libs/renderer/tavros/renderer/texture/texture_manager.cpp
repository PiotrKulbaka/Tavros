#include <tavros/renderer/texture/texture_manager.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/math.hpp>
#include <tavros/core/exception.hpp>

#include <tavros/renderer/rhi/string_utils.hpp>
#include <tavros/renderer/texture/mipmap_generator.hpp>
#include <tavros/renderer/texture/texture_uploader.hpp>
#include <tavros/renderer/resource_manager.hpp>

namespace
{
    tavros::core::logger logger("texture_manager");

    tavros::assets::image::pixel_format to_im_format(tavros::renderer::rhi::pixel_format fmt) noexcept
    {
        using ipf = tavros::assets::image::pixel_format;
        using rpf = tavros::renderer::rhi::pixel_format;
        switch (fmt) {
        case rpf::r8un:
            return ipf::r8;
        case rpf::rg8un:
            return ipf::rg8;
        case rpf::rgb8un:
            return ipf::rgb8;
        case rpf::rgba8un:
            return ipf::rgba8;
        case rpf::none:
            return ipf::none;
        default:
            return ipf::none;
        }
    }
} // namespace

namespace tavros::renderer
{

    texture_manager::texture_manager(rhi::graphics_device* gdevice, assets::asset_manager* am) noexcept
        : m_gdevice(gdevice)
        , m_am(am)
    {
    }

    texture_manager::~texture_manager() noexcept
    {
        clear();
    }

    texture_manager::resource_ref_type texture_manager::load(upload_context& upctx, core::string_view name, tef::workspace& ws)
    {
        if (auto res = find(name); res) {
            // Already loaded
            acquire(res);
            return res;
        }

        texture_desc      desc;
        core::diagnostics ds;

        tavros::tef::schema<texture_desc>::deserialize(ws.resolve_path(name), desc, ds);
        if (ds.error_count() > 0 || ds.fatal_count() > 0) {
            logger.error("Failed to load named texture '{}'", name);
            logger.flush(ds);
            return {};
        }

        if (ds.total_count() > 0) {
            logger.flush(ds);
        }

        return load(upctx, desc);
    }

    texture_manager::resource_ref_type texture_manager::load(upload_context& upctx, const texture_desc& desc)
    {
        auto key = desc.name();
        if (auto res = find(key); res) {
            // Already loaded
            acquire(res);
            return res;
        }

        // Decode image on CPU
        assets::image im;
        try {
            auto im_fmt = to_im_format(desc.load_params().pixel_format);
            auto data = m_am->read_binary(desc.load_params().path);
            im = assets::image::decode(data, im_fmt, true);
        } catch (const core::file_error& e) {
            logger.error("Failed to open image '{}'", desc.load_params().path);
            return {};
        }

        auto tex = texture(m_gdevice, upctx, im, desc, true);
        if (tex.is_valid()) {
            auto ref = insert(std::move(tex));
            if (!ref) {
                logger.error("Failed to insert texture '{}'", key);
            }
            return ref;
        }

        logger.error("Failed to create texture '{}'", key);
        return {};
    }

    texture_manager::resource_ref_type texture_manager::load(upload_context& upctx, assets::image_view im, const texture_desc& desc)
    {
        auto key = desc.name();
        if (auto res = find(key); res) {
            // Already loaded
            acquire(res);
            return res;
        }

        auto tex = texture(m_gdevice, upctx, im, desc, true);
        if (tex.is_valid()) {
            auto ref = insert(std::move(tex));
            if (!ref) {
                logger.error("Failed to insert texture '{}'", key);
            }
            return ref;
        }

        logger.error("Failed to create texture '{}'", key);
        return {};
    }

} // namespace tavros::renderer
