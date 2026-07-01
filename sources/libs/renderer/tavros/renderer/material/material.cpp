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

    material::material(rhi::graphics_device* gdevice, const material_desc& desc, shader_loader& sl, core::buffer_view<vertex_attribute> vert_attribs, uint32 msaa, rhi::pixel_format ds_format)
        : m_gdevice(gdevice)
    {
        ::logger.debug("Creating material '{}'", desc.name());

        // 1. Load & compile shaders
        auto vs_src = sl.load(desc.shaders().vertex_shader_path, {tavros::renderer::shader_language::glsl_460, ""});
        auto fs_src = sl.load(desc.shaders().fragment_shader_path, {tavros::renderer::shader_language::glsl_460, ""});

        auto sh = m_gdevice->create_shader({vs_src, fs_src});
        if (!sh) {
            ::logger.error(
                "Failed to create material '{}': shader compilation failed (VS='{}', FS='{}')",
                desc.name(),
                desc.shaders().vertex_shader_path,
                desc.shaders().fragment_shader_path
            );
            return;
        }

        // 2. Reflect
        const auto* reflect = m_gdevice->get_shader_reflect_ptr(sh);
        TAV_ASSERT(reflect);

        // 3. Validate: every name in desc must exist in reflect outputs
        // desc names absent in reflect = error (referencing non-existent output)
        // reflect outputs absent in desc = ok (material simply does not configure that attachment)
        bool valid = true;
        for (const auto& cas : desc.color_attachment_states()) {
            bool found = false;
            for (const auto& out : reflect->outputs()) {
                if (out.name == cas.name) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                ::logger.error(
                    "Failed to create material '{}': color_attachment_state '{}' "
                    "has no corresponding output in shader (VS='{}', FS='{}')",
                    desc.name(), cas.name,
                    desc.shaders().vertex_shader_path,
                    desc.shaders().fragment_shader_path
                );
                valid = false;
            }
        }

        if (!valid) {
            m_gdevice->safe_destroy(sh);
            return;
        }

        // 4. Sort reflect outputs by location
        // Locations may be sparse and non-sequential (e.g. 0, 5, 12345).
        // We sort so we can iterate from lowest to highest location.
        std::vector<const rhi::output_reflect*> sorted_outputs;
        sorted_outputs.reserve(reflect->outputs().size());
        for (const auto& out : reflect->outputs()) {
            sorted_outputs.push_back(&out);
        }
        std::sort(sorted_outputs.begin(), sorted_outputs.end(), [](const rhi::output_reflect* a, const rhi::output_reflect* b) {
            return a->location < b->location;
        });

        // 5. Check for duplicate locations
        for (size_t i = 1; i < sorted_outputs.size(); ++i) {
            if (sorted_outputs[i]->location == sorted_outputs[i - 1]->location) {
                ::logger.error(
                    "Failed to create material '{}': shader has two outputs at the same location {} ('{}' and '{}')",
                    desc.name(),
                    sorted_outputs[i]->location,
                    sorted_outputs[i - 1]->name,
                    sorted_outputs[i]->name
                );
                valid = false;
            }
        }

        if (!valid) {
            m_gdevice->safe_destroy(sh);
            return;
        }

        // Check and collect vertex attributes
        struct attrib_info
        {
            const rhi::vertex_attribute_reflect* reflect;
            const vertex_attribute*              attr;
        };
        core::fixed_vector<attrib_info, rhi::k_max_vertex_attributes> attr_infos;

        for (const auto& reflected : reflect->vertex_attributes()) {
            bool found = false;
            for (const auto& attrib : vert_attribs) {
                if (attrib.name == reflected.name) {
                    found = true;
                    attr_infos.push_back(attrib_info{&reflected, &attrib});
                    break;
                }
            }
            if (!found) {
                ::logger.error(
                    "Failed to create material '{}': shader vertex attribute '{}' at location {} "
                    "has no matching entry in vert_attribs",
                    desc.name(), reflected.name, reflected.location
                );
                valid = false;
            }
        }

        if (!valid) {
            m_gdevice->safe_destroy(sh);
            return;
        }

        // 6. Build pipeline_create_info
        // We iterate sorted outputs and emit one color_attachment_state per output,
        // in location order. Gaps between locations get dummy (off) slots so that
        // the index in color_attachments[] matches the shader location.
        rhi::pipeline_create_info info;
        info.shader_program = sh;

        // Attrib bindings
        for (const auto& ai : attr_infos) {
            rhi::vertex_attribute attr;
            attr.format = ai.reflect->format;
            attr.type = ai.reflect->type;
            attr.normalize = false;
            attr.location = ai.reflect->location;
            attr.stride = ai.attr->stride;
            attr.offset = ai.attr->offset;
            attr.instance_divisor = ai.attr->instance_divisor;

            info.bindings.push_back(attr);
        }


        constexpr auto blend_off = rhi::blend_state{false};
        constexpr auto mask_off = tavros::core::flags<rhi::color_mask>();

        uint32 next_location = 0;
        for (const auto* out : sorted_outputs) {
            // Fill gap slots between next_location and this output's location
            while (next_location < out->location) {
                ::logger.warning(
                    "material '{}': no shader output at location {} (gap) -- inserting dummy slot",
                    desc.name(), next_location
                );
                info.color_attachments.push_back(rhi::color_attachment_state{rhi::pixel_format::none, mask_off, blend_off});
                ++next_location;
            }

            // Find matching desc config for this output (optional)
            const material_desc::color_attachment_state_config* matched_desc = nullptr;
            for (const auto& cas : desc.color_attachment_states()) {
                if (cas.name == out->name) {
                    matched_desc = &cas;
                    break;
                }
            }

            if (matched_desc) {
                rhi::color_attachment_state cas;
                cas.format = out->format;
                cas.mask = matched_desc->mask;
                cas.blend = matched_desc->blend;
                info.color_attachments.push_back(cas);
            } else {
                // Output exists in shader but has no desc config -- write disabled
                ::logger.debug(
                    "material '{}': shader output '{}' at location {} has no config in desc -- write disabled",
                    desc.name(), out->name, out->location
                );
                info.color_attachments.push_back(rhi::color_attachment_state{out->format, mask_off, blend_off});
            }

            ++next_location;
        }

        // 7. Depth/stencil
        info.depth_stencil_attachment.format = ds_format;
        info.depth_stencil_attachment.depth_test_enable = desc.depth_attachment_state().test_enabled;
        info.depth_stencil_attachment.depth_write_enable = desc.depth_attachment_state().write_enabled;
        info.depth_stencil_attachment.depth_compare = desc.depth_attachment_state().compare_op;
        info.depth_stencil_attachment.stencil_test_enable = desc.stencil_attachment_state().test_enabled;
        info.depth_stencil_attachment.stencil_front = desc.stencil_attachment_state().front;
        info.depth_stencil_attachment.stencil_back = desc.stencil_attachment_state().back;

        // 8. Topology / rasterizer / multisample
        info.topology = desc.topology().topology;
        info.rasterizer = desc.rasterizer();

        info.multisample.sample_count = msaa;
        info.multisample.sample_shading_enabled = false;
        info.multisample.min_sample_shading = 1.0f;

        // 9. Create pipeline
        auto pipeline = m_gdevice->create_pipeline(info);
        if (!pipeline) {
            ::logger.error(
                "Failed to create material '{}': pipeline creation failed",
                desc.name()
            );
            m_gdevice->safe_destroy(sh);
            return;
        }

        ::logger.info("Material '{}' created successfully", desc.name());

        m_pipeline = pipeline;
        m_shader = sh;
    }

    material::material(material&& other) noexcept
        : m_gdevice(other.m_gdevice)
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
