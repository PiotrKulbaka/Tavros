#include <tavros/renderer/material/material.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/debug/assert.hpp>
#include <tavros/core/exception.hpp>

#include <tavros/renderer/rhi/string_utils.hpp>

namespace
{
    tavros::core::logger logger("material");
} // namespace

namespace tavros::renderer
{

    material::material(rhi::graphics_device* gdevice, const material_desc& desc, shader_loader& sl, uint32 msaa, rhi::pixel_format ds_format)
        : basic_resource(desc.name())
        , m_gdevice(gdevice)
    {
        auto vs = sl.load(desc.shaders().vertex_shader_path, {tavros::renderer::shader_language::glsl_460, ""});
        auto fs = sl.load(desc.shaders().fragment_shader_path, {tavros::renderer::shader_language::glsl_460, ""});

        auto sh = m_gdevice->create_shader({vs, fs});
        if (!sh) {
            ::logger.error(
                "Failed to create material: shader is not compiled (\"{}\", \"{}\")",
                desc.shaders().vertex_shader_path,
                desc.shaders().fragment_shader_path
            );
            return;
        }

        const auto* reflect = m_gdevice->get_shader_reflect_ptr(sh);
        TAV_ASSERT(reflect);

        // Prepare color attachments combined info
        struct color_attachment_combined_info
        {
            const rhi::output_reflect*                          reflect = nullptr;
            const material_desc::color_attachment_state_config* desc = nullptr;
        };
        color_attachment_combined_info ca_combined[rhi::k_max_color_attachments];
        size_t                         ca_combined_size = 0;

        for (const auto& out : reflect->outputs()) {
            TAV_ASSERT(out.location < rhi::k_max_color_attachments);
            ca_combined[out.location].reflect = &out;
            for (const auto& cas : desc.color_attachment_states()) {
                if (cas.name == out.name) {
                    ca_combined[out.location].desc = &cas;
                    break;
                }
            }
            if (ca_combined_size <= out.location) {
                ca_combined_size = out.location + 1;
            }
        }

        // Validate color attachments combined info
        for (size_t i = 0; i < ca_combined_size; ++i) {
            const auto* cur = &ca_combined[i];
            if (cur->desc) {
                if (!cur->reflect) {
                    ::logger.error(
                        "Failed to create material: shader has no attachment '{}'", cur->desc->name
                    );
                    m_gdevice->safe_destroy(sh);
                    return;
                }
            }
        }

        rhi::pipeline_create_info info;

        // Shader program
        info.shader_program = sh;

        // Color attachments
        for (size_t i = 0; i < ca_combined_size; ++i) {
            constexpr auto blend_off = rhi::blend_state{false};
            constexpr auto mask_off = tavros::core::flags<rhi::color_mask>();

            const auto* cur = &ca_combined[i];
            if (cur->desc) {
                TAV_ASSERT(cur->reflect);
                rhi::color_attachment_state cas;
                cas.format = cur->reflect->format;
                cas.mask = cur->desc->mask;
                cas.blend = cur->desc->blend;
                info.color_attachments.push_back(cas);
            } else {
                info.color_attachments.push_back(rhi::color_attachment_state{cur->reflect->format, mask_off, blend_off});
            }
        }

        // Depth stencil
        info.depth_stencil_attachment.format = ds_format;
        info.depth_stencil_attachment.depth_test_enable = desc.depth_attachment_state().test_enabled;
        info.depth_stencil_attachment.depth_write_enable = desc.depth_attachment_state().write_enabled;
        info.depth_stencil_attachment.depth_compare = desc.depth_attachment_state().compare_op;
        info.depth_stencil_attachment.stencil_test_enable = desc.stencil_attachment_state().test_enabled;
        info.depth_stencil_attachment.stencil_front = desc.stencil_attachment_state().front;
        info.depth_stencil_attachment.stencil_back = desc.stencil_attachment_state().back;

        // Topology
        info.topology = desc.topology().topology;

        // Rasterizer
        info.rasterizer = desc.rasterizer();

        // Multisample
        info.multisample.sample_count = msaa;
        info.multisample.sample_shading_enabled = false;
        info.multisample.min_sample_shading = 1.0f;

        auto pipeline = m_gdevice->create_pipeline(info);
        if (!pipeline) {
            logger.error("Failed to create material: pipeline not created");
            m_gdevice->safe_destroy(sh);
            return;
        }

        logger.debug(
            "Loaded {}", desc.name()
        );

        m_pipeline = pipeline;
        m_shader = sh;
        set_valid();
    }

    material::material(material&& other) noexcept
        : basic_resource(std::move(other))
        , m_gdevice(other.m_gdevice)
        , m_pipeline(other.m_pipeline)
        , m_shader(other.m_shader)
    {
        other.m_gdevice = nullptr;
        other.m_pipeline = {};
        other.m_shader = {};
    }

    material::~material() noexcept
    {
        if (m_pipeline) {
            m_gdevice->safe_destroy(m_pipeline);
        }
        if (m_shader) {
            m_gdevice->safe_destroy(m_shader);
        }
    }

    rhi::pipeline_handle material::gpu_pipeline() const noexcept
    {
        return m_pipeline;
    }

    const rhi::shader_reflect* material::shader_reflect() const noexcept
    {
        return m_gdevice->get_shader_reflect_ptr(m_shader);
    }

} // namespace tavros::renderer
