#include <tavros/renderer/internal/opengl/gl_shader_program_reflect.hpp>

#include <tavros/renderer/internal/opengl/type_conversions.hpp>
#include <tavros/renderer/internal/opengl/gl_check.hpp>
#include <tavros/core/debug/assert.hpp>
#include <tavros/core/logger/logger.hpp>

#include <algorithm>

namespace
{
    tavros::core::logger logger("gl_shader_program_reflect");

    constexpr size_t k_max_name_len = 62;

    namespace rhi = tavros::renderer::rhi;

    template<class T>
    using vector = tavros::core::vector<T>;

    // -----------------------------------------------------------------------
    //  Internal helper types
    // -----------------------------------------------------------------------

    struct rhi_sh_res_t
    {
        bool                      valid = false;
        rhi::shader_resource_type type = rhi::shader_resource_type::sampler_2d;
    };

    struct rhi_output_type_t
    {
        bool                  valid = false;
        rhi::scalar_type      type = rhi::scalar_type::f32;
        rhi::composite_format format = rhi::composite_format::vec4;
    };

    // -----------------------------------------------------------------------
    //  GL type -> RHI opaque type
    // -----------------------------------------------------------------------
    rhi_sh_res_t to_rhi_opaque_type(GLenum type) noexcept
    {
        using t = rhi::shader_resource_type;
        switch (type) {
        case GL_SAMPLER_2D:
            return {true, t::sampler_2d};
        case GL_SAMPLER_2D_SHADOW:
            return {true, t::sampler_2d_shadow};
        case GL_SAMPLER_2D_ARRAY:
            return {true, t::sampler_2d_array};
        case GL_SAMPLER_2D_ARRAY_SHADOW:
            return {true, t::sampler_2d_array_shadow};
        case GL_UNSIGNED_INT_SAMPLER_2D:
            return {true, t::usampler_2d};
        case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
            return {true, t::usampler_2d_array};
        case GL_INT_SAMPLER_2D:
            return {true, t::isampler_2d};
        case GL_INT_SAMPLER_2D_ARRAY:
            return {true, t::isampler_2d_array};
        case GL_SAMPLER_3D:
            return {true, t::sampler_3d};
        case GL_UNSIGNED_INT_SAMPLER_3D:
            return {true, t::usampler_3d};
        case GL_INT_SAMPLER_3D:
            return {true, t::isampler_3d};
        case GL_SAMPLER_CUBE:
            return {true, t::sampler_cube};
        case GL_SAMPLER_CUBE_SHADOW:
            return {true, t::sampler_cube_shadow};
        case GL_SAMPLER_CUBE_MAP_ARRAY:
            return {true, t::sampler_cube_array};
        case GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW:
            return {true, t::sampler_cube_array_shadow};
        case GL_UNSIGNED_INT_SAMPLER_CUBE:
            return {true, t::usampler_cube};
        case GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY:
            return {true, t::usampler_cube_array};
        case GL_INT_SAMPLER_CUBE:
            return {true, t::isampler_cube};
        case GL_INT_SAMPLER_CUBE_MAP_ARRAY:
            return {true, t::isampler_cube_array};
        case GL_SAMPLER_BUFFER:
            return {true, t::sampler_buffer};
        case GL_UNSIGNED_INT_SAMPLER_BUFFER:
            return {true, t::usampler_buffer};
        case GL_INT_SAMPLER_BUFFER:
            return {true, t::isampler_buffer};
        case GL_IMAGE_2D:
            return {true, t::image_2d};
        case GL_IMAGE_2D_ARRAY:
            return {true, t::image_2d_array};
        case GL_UNSIGNED_INT_IMAGE_2D:
            return {true, t::uimage_2d};
        case GL_UNSIGNED_INT_IMAGE_2D_ARRAY:
            return {true, t::uimage_2d_array};
        case GL_INT_IMAGE_2D:
            return {true, t::iimage_2d};
        case GL_INT_IMAGE_2D_ARRAY:
            return {true, t::iimage_2d_array};
        case GL_IMAGE_3D:
            return {true, t::image_3d};
        case GL_UNSIGNED_INT_IMAGE_3D:
            return {true, t::uimage_3d};
        case GL_INT_IMAGE_3D:
            return {true, t::iimage_3d};
        case GL_IMAGE_CUBE:
            return {true, t::image_cube};
        case GL_IMAGE_CUBE_MAP_ARRAY:
            return {true, t::image_cube_array};
        case GL_UNSIGNED_INT_IMAGE_CUBE:
            return {true, t::uimage_cube};
        case GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY:
            return {true, t::uimage_cube_array};
        case GL_INT_IMAGE_CUBE:
            return {true, t::iimage_cube};
        case GL_INT_IMAGE_CUBE_MAP_ARRAY:
            return {true, t::iimage_cube_array};
        case GL_IMAGE_BUFFER:
            return {true, t::image_buffer};
        case GL_UNSIGNED_INT_IMAGE_BUFFER:
            return {true, t::uimage_buffer};
        case GL_INT_IMAGE_BUFFER:
            return {true, t::iimage_buffer};
        default:
            return {false};
        }
    }

    // -----------------------------------------------------------------------
    //  GL type -> RHI output type
    // -----------------------------------------------------------------------
    rhi_output_type_t to_rhi_output_type(GLenum type) noexcept
    {
        using t = rhi::scalar_type;
        using f = rhi::composite_format;
        switch (type) {
        case GL_FLOAT:
            return {true, t::f32, f::scalar};
        case GL_INT:
            return {true, t::i32, f::scalar};
        case GL_UNSIGNED_INT:
            return {true, t::u32, f::scalar};
        case GL_DOUBLE:
            return {true, t::f64, f::scalar};
        case GL_FLOAT_VEC2:
            return {true, t::f32, f::vec2};
        case GL_FLOAT_VEC3:
            return {true, t::f32, f::vec3};
        case GL_FLOAT_VEC4:
            return {true, t::f32, f::vec4};
        case GL_INT_VEC2:
            return {true, t::i32, f::vec2};
        case GL_INT_VEC3:
            return {true, t::i32, f::vec3};
        case GL_INT_VEC4:
            return {true, t::i32, f::vec4};
        case GL_UNSIGNED_INT_VEC2:
            return {true, t::u32, f::vec2};
        case GL_UNSIGNED_INT_VEC3:
            return {true, t::u32, f::vec3};
        case GL_UNSIGNED_INT_VEC4:
            return {true, t::u32, f::vec4};
        case GL_DOUBLE_VEC2:
            return {true, t::f64, f::vec2};
        case GL_DOUBLE_VEC3:
            return {true, t::f64, f::vec3};
        case GL_DOUBLE_VEC4:
            return {true, t::f64, f::vec4};
        default:
            return {false};
        }
    }

    // -----------------------------------------------------------------------
    //  Helper: strip block name prefix from member name
    //  "Scene.camera_pos" -> "camera_pos"
    //  "Scene.cl[0].color" -> "cl[0].color"
    // -----------------------------------------------------------------------
    tavros::core::string_view strip_block_prefix(tavros::core::string_view full_name) noexcept
    {
        auto dot = full_name.find('.');
        if (dot != tavros::core::string_view::npos) {
            return full_name.substr(dot + 1);
        }
        return full_name;
    }

    // -----------------------------------------------------------------------
    //  Helper: count active resources matching a predicate over GL interface
    // -----------------------------------------------------------------------
    GLint count_program_inputs_with_location(GLuint prog, GLenum interface) noexcept
    {
        GLint total = 0;
        GL_CALL(glGetProgramInterfaceiv(prog, interface, GL_ACTIVE_RESOURCES, &total));

        GLint result = 0;
        for (GLint i = 0; i < total; ++i) {
            GLenum props[] = {GL_LOCATION};
            GLint  vals[1] = {};
            GL_CALL(glGetProgramResourceiv(prog, interface, i, 1, props, 1, nullptr, vals));
            if (vals[0] >= 0) {
                ++result;
            }
        }
        return result;
    }

    // -----------------------------------------------------------------------
    //  fill_vertex_attributes
    // -----------------------------------------------------------------------
    bool fill_vertex_attributes(GLuint prog, vector<rhi::vertex_attribute_reflect>& out) noexcept
    {
        tavros::core::fixed_string<1024> name;

        GLint total = 0;
        GL_CALL(glGetProgramInterfaceiv(prog, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, &total));

        // Pre-count non-builtin attributes to reserve
        out.reserve(static_cast<size_t>(count_program_inputs_with_location(prog, GL_PROGRAM_INPUT)));

        bool success = true;

        for (GLint i = 0; i < total; ++i) {
            GLenum props[] = {GL_TYPE, GL_ARRAY_SIZE, GL_LOCATION};
            GLint  vals[3] = {};
            GL_CALL(glGetProgramResourceiv(prog, GL_PROGRAM_INPUT, i, 3, props, 3, nullptr, vals));

            // Skip built-ins (gl_VertexID, gl_InstanceID, etc.)
            if (vals[2] < 0) {
                continue;
            }

            GLsizei len = 0;
            GL_CALL(glGetProgramResourceName(prog, GL_PROGRAM_INPUT, i, name.capacity(), &len, name.data()));
            name.set_size(tavros::core::unsafe, static_cast<size_t>(len + 1));

            if (static_cast<size_t>(len) > k_max_name_len) {
                logger.error("[vertex_attributes] Attribute name '{}' exceeds max length ({} > {}).", name, len, k_max_name_len);
                success = false;
                continue;
            }

            auto t = rhi::gl_type_to_rhi_type(static_cast<GLenum>(vals[0]));
            auto array_size = static_cast<uint32>(vals[1]);
            auto location = static_cast<uint32>(vals[2]);

            out.push_back({{name.data(), static_cast<size_t>(len)}, t.format, t.type, array_size, location});
        }

        // Sort by location for deterministic order
        if (success) {
            std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) {
                return a.location < b.location;
            });
        }

        return success;
    }


    // -----------------------------------------------------------------------
    //  fill_shader_resources  (standalone opaque uniforms - samplers + images)
    // -----------------------------------------------------------------------

    bool fill_shader_resources(GLuint prog, vector<rhi::shader_resource_reflect>& out) noexcept
    {
        tavros::core::fixed_string<1024> name;

        GLint total = 0;
        GL_CALL(glGetProgramInterfaceiv(prog, GL_UNIFORM, GL_ACTIVE_RESOURCES, &total));

        // Pre-count standalone opaque uniforms to reserve
        {
            GLint standalone = 0;
            for (GLint i = 0; i < total; ++i) {
                GLenum props[] = {GL_TYPE, GL_LOCATION, GL_BLOCK_INDEX};
                GLint  vals[3] = {};
                glGetProgramResourceiv(prog, GL_UNIFORM, i, 3, props, 3, nullptr, vals);

                if (vals[1] < 0) {
                    continue; // no location -> built-in or block member
                }
                if (vals[2] >= 0) {
                    continue; // part of a block -> skip
                }
                if (to_rhi_opaque_type(static_cast<GLenum>(vals[0])).valid) {
                    ++standalone;
                }
            }
            out.reserve(static_cast<size_t>(standalone));
        }

        bool success = true;

        for (GLint i = 0; i < total; ++i) {
            GLenum props[] = {GL_TYPE, GL_ARRAY_SIZE, GL_LOCATION, GL_BLOCK_INDEX};
            GLint  vals[4] = {};
            GL_CALL(glGetProgramResourceiv(prog, GL_UNIFORM, i, 4, props, 4, nullptr, vals));

            // Skip block members
            if (vals[3] >= 0) {
                continue;
            }

            // Skip uniforms without location (built-ins)
            if (vals[2] < 0) {
                continue;
            }

            auto gl_type = static_cast<GLenum>(vals[0]);
            auto t = to_rhi_opaque_type(gl_type);

            // Skip non-opaque types (plain float/vec/mat - should live in UBO)
            if (!t.valid) {
                logger.error("[shader_resources] Unsupported type of resource '{}'.", name);
                success = false;
                continue;
            }

            GLsizei len = 0;
            GL_CALL(glGetProgramResourceName(prog, GL_UNIFORM, i, name.capacity(), &len, name.data()));
            name.set_size(tavros::core::unsafe, static_cast<size_t>(len + 1));

            if (static_cast<size_t>(len) > k_max_name_len) {
                logger.error("[shader_resources] Resource name '{}' exceeds max length ({} > {}).", name, len, k_max_name_len);
                success = false;
                continue;
            }

            // Read the actual binding from the uniform value (set via layout(binding=N))
            GLint binding = 0;
            GL_CALL(glGetUniformiv(prog, vals[2], &binding));

            if (binding < 0) {
                logger.error("[shader_resources] Resource '{}' has no binding point assigned.", name);
                success = false;
                continue;
            }

            out.push_back({{name.data(), static_cast<size_t>(len)}, t.type, static_cast<uint32>(binding)});
        }

        // Sort by binding for deterministic order
        if (success) {
            std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) {
                return a.binding < b.binding;
            });
        }

        return success;
    }


    // -----------------------------------------------------------------------
    //  fill_ubos
    //
    //  Fills three parallel arrays:
    //    blocks  - one entry per UBO block (name, size, binding)
    //    members - all members of all blocks, packed contiguously per block
    //    ranges  - per-block [begin, end) index into members array
    //
    //  Member names have the block prefix stripped:
    //    "Scene.camera_pos" -> "camera_pos"
    //    "Scene.cl[0].color" -> "cl[0].color"  (array indices preserved)
    //
    //  Members within each block are sorted by offset.
    // -----------------------------------------------------------------------
    using ubo_range = tavros::renderer::rhi::gl_shader_program_reflect::ubo_range;

    bool fill_ubos(
        GLuint                               prog,
        vector<rhi::constant_block_reflect>& blocks,
        vector<rhi::member_reflect>&         members,
        vector<ubo_range>&                   ranges
    ) noexcept
    {
        tavros::core::fixed_string<1024> block_name_buf;
        tavros::core::fixed_string<1024> member_name_buf;

        bool success = true;

        // -----------------------------------------------------------------------
        //  Step 1: count blocks and total members to reserve upfront
        // -----------------------------------------------------------------------

        GLint block_count = 0;
        GL_CALL(glGetProgramInterfaceiv(prog, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &block_count));

        GLint total_members = 0;
        for (GLint bi = 0; bi < block_count; ++bi) {
            GLenum props[] = {GL_NUM_ACTIVE_VARIABLES};
            GLint  vals[1] = {};
            GL_CALL(glGetProgramResourceiv(prog, GL_UNIFORM_BLOCK, bi, 1, props, 1, nullptr, vals));
            total_members += vals[0];
        }

        blocks.reserve(static_cast<size_t>(block_count));
        members.reserve(static_cast<size_t>(total_members));
        ranges.reserve(static_cast<size_t>(block_count));

        // -----------------------------------------------------------------------
        //  Step 2: iterate blocks
        // -----------------------------------------------------------------------

        for (GLint bi = 0; bi < block_count; ++bi) {
            // -- Block metadata --
            GLsizei block_name_len = 0;
            GL_CALL(glGetProgramResourceName(prog, GL_UNIFORM_BLOCK, bi, block_name_buf.capacity(), &block_name_len, block_name_buf.data()));
            block_name_buf.set_size(tavros::core::unsafe, static_cast<size_t>(block_name_len + 1));

            if (static_cast<size_t>(block_name_len) > k_max_name_len) {
                logger.error("[ubos] Block name '{}' exceeds max length ({} > {}).", block_name_buf, block_name_len, k_max_name_len);
                success = false;
                // Still iterate to keep ranges aligned with blocks
            }

            GLenum block_props[] = {GL_BUFFER_BINDING, GL_BUFFER_DATA_SIZE, GL_NUM_ACTIVE_VARIABLES};
            GLint  block_vals[3] = {};
            GL_CALL(glGetProgramResourceiv(prog, GL_UNIFORM_BLOCK, bi, 3, block_props, 3, nullptr, block_vals));

            auto block_binding = static_cast<uint32>(block_vals[0]);
            auto block_size = static_cast<uint32>(block_vals[1]);
            auto member_count = static_cast<GLint>(block_vals[2]);

            // -- Record range begin before pushing members --
            auto range_begin = static_cast<uint32>(members.size());

            // -- Fetch member indices for this block --
            vector<GLint> member_indices(static_cast<size_t>(member_count));
            {
                GLenum active_vars_prop = GL_ACTIVE_VARIABLES;
                GL_CALL(glGetProgramResourceiv(prog, GL_UNIFORM_BLOCK, bi, 1, &active_vars_prop, member_count, nullptr, member_indices.data()));
            }

            // -- Collect members into a temporary for sorting --
            struct raw_member
            {
                rhi::member_reflect data;
                uint32              offset = 0; // duplicate for sort key
            };
            vector<raw_member> raw_members;
            raw_members.reserve(static_cast<size_t>(member_count));

            for (GLint mi : member_indices) {
                GLenum m_props[] = {
                    GL_TYPE,
                    GL_OFFSET,
                    GL_ARRAY_SIZE,
                    GL_ARRAY_STRIDE,
                    GL_MATRIX_STRIDE
                };
                GLint m_vals[5] = {};
                GL_CALL(glGetProgramResourceiv(prog, GL_UNIFORM, mi, 5, m_props, 5, nullptr, m_vals));

                GLsizei member_name_len = 0;
                GL_CALL(glGetProgramResourceName(prog, GL_UNIFORM, mi, member_name_buf.capacity(), &member_name_len, member_name_buf.data()));
                member_name_buf.set_size(tavros::core::unsafe, static_cast<size_t>(member_name_len + 1));

                // Strip "BlockName." prefix - keep only the member part
                auto stripped = strip_block_prefix({member_name_buf.data(), static_cast<size_t>(member_name_len)});

                if (stripped.size() > k_max_name_len) {
                    logger.error("[ubos] Member name '{}' in block '{}' exceeds max length ({} > {}).", stripped, block_name_buf, stripped.size(), k_max_name_len);
                    success = false;
                    continue;
                }

                auto t = rhi::gl_type_to_rhi_type(static_cast<GLenum>(m_vals[0]));
                auto offset = static_cast<uint32>(m_vals[1]);
                auto array_size = static_cast<uint32>(m_vals[2]);
                auto array_stride = static_cast<uint32>(m_vals[3]);

                // array_size == 1 and array_stride == 0 -> not an array
                // Normalize: store 0 for array_size when it's a plain scalar/vector/matrix
                if (array_stride == 0) {
                    array_size = 0;
                }

                rhi::member_reflect m;
                m.name.assign(stripped.data(), stripped.size());
                m.format = t.format;
                m.type = t.type;
                m.offset = offset;
                m.array_size = array_size;
                m.array_stride = array_stride;

                raw_members.push_back({m, offset});
            }

            // Sort members by offset - deterministic, matches layout order
            std::sort(raw_members.begin(), raw_members.end(), [](const auto& a, const auto& b) {
                return a.offset < b.offset;
            });

            // Push sorted members into the flat output array
            for (auto& rm : raw_members) {
                members.push_back(rm.data);
            }

            // -- Record range end --
            auto range_end = static_cast<uint32>(members.size());

            // -- Push block and its range --
            if (success) {
                blocks.push_back({{block_name_buf.data(), static_cast<size_t>(block_name_len)}, block_size, block_binding});
            }
            ranges.push_back({range_begin, range_end});
        }

        return success;
    }

    // -----------------------------------------------------------------------
    //  fill_ssbos
    // -----------------------------------------------------------------------
    bool fill_ssbos(GLuint prog, vector<rhi::storage_block_reflect>& out) noexcept
    {
        tavros::core::fixed_string<1024> name;

        GLint count = 0;
        GL_CALL(glGetProgramInterfaceiv(prog, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &count));

        out.reserve(static_cast<size_t>(count));

        bool success = true;

        for (GLint i = 0; i < count; ++i) {
            GLsizei len = 0;
            GL_CALL(glGetProgramResourceName(prog, GL_SHADER_STORAGE_BLOCK, i, name.capacity(), &len, name.data()));
            name.set_size(tavros::core::unsafe, static_cast<size_t>(len + 1));

            if (static_cast<size_t>(len) > k_max_name_len) {
                logger.error("[ssbos] Storage block name '{}' exceeds max length ({} > {}).", name, len, k_max_name_len);
                success = false;
                continue;
            }

            GLenum props[] = {GL_BUFFER_BINDING, GL_BUFFER_DATA_SIZE};
            GLint  vals[2] = {};
            GL_CALL(glGetProgramResourceiv(prog, GL_SHADER_STORAGE_BLOCK, i, 2, props, 2, nullptr, vals));

            auto binding = static_cast<uint32>(vals[0]);
            auto size = static_cast<uint32>(vals[1]);

            out.push_back({{name.data(), static_cast<size_t>(len)}, size, binding});
        }

        // Sort by binding for deterministic order
        if (success) {
            std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) {
                return a.binding < b.binding;
            });
        }

        return success;
    }

    // -----------------------------------------------------------------------
    //  fill_outputs
    // -----------------------------------------------------------------------
    bool fill_outputs(GLuint prog, vector<rhi::output_reflect>& out) noexcept
    {
        tavros::core::fixed_string<1024> name;

        GLint total = 0;
        GL_CALL(glGetProgramInterfaceiv(prog, GL_PROGRAM_OUTPUT, GL_ACTIVE_RESOURCES, &total));

        // Pre-count non-builtin outputs to reserve
        out.reserve(static_cast<size_t>(count_program_inputs_with_location(prog, GL_PROGRAM_OUTPUT)));

        bool success = true;

        for (GLint i = 0; i < total; ++i) {
            GLenum props[] = {GL_TYPE, GL_ARRAY_SIZE, GL_LOCATION};
            GLint  vals[3] = {};
            GL_CALL(glGetProgramResourceiv(prog, GL_PROGRAM_OUTPUT, i, 3, props, 3, nullptr, vals));

            // Skip built-in outputs (gl_FragDepth, gl_FragCoord, etc.)
            if (vals[2] < 0) {
                continue;
            }

            GLsizei len = 0;
            GL_CALL(glGetProgramResourceName(prog, GL_PROGRAM_OUTPUT, i, name.capacity(), &len, name.data()));
            name.set_size(tavros::core::unsafe, static_cast<size_t>(len + 1));

            if (static_cast<size_t>(len) > k_max_name_len) {
                logger.error("[outputs] Output name '{}' exceeds max length ({} > {}).", name, len, k_max_name_len);
                success = false;
                continue;
            }

            auto t = to_rhi_output_type(static_cast<GLenum>(vals[0]));
            if (!t.valid) {
                logger.error(
                    "[outputs] Output '{}' has unsupported type (GL enum = 0x{:X}). "
                    "Only float/int/uint scalar and vector types are valid fragment outputs.",
                    name, static_cast<uint32>(vals[0])
                );
                success = false;
                continue;
            }

            auto location = static_cast<uint32>(vals[2]);
            out.push_back({{name.data(), static_cast<size_t>(len)}, t.format, t.type, location});
        }

        // Sort by location for deterministic order
        if (success) {
            std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) {
                return a.location < b.location;
            });
        }

        return success;
    }
} // namespace

namespace tavros::renderer::rhi
{

    gl_shader_program_reflect::gl_shader_program_reflect(GLuint prog, bool is_compute)
    {
        bool success = true;
        success &= fill_vertex_attributes(prog, m_vert_attribs);
        success &= fill_shader_resources(prog, m_shader_res);
        success &= fill_ubos(prog, m_ubo_blocks, m_ubo_members, m_ubo_ranges);
        success &= fill_ssbos(prog, m_ssbo_blocks);
        success &= fill_outputs(prog, m_outputs);

        m_compute.is_compute = is_compute;
        if (is_compute) {
            GLint wg[3] = {};
            GL_CALL(glGetProgramiv(prog, GL_COMPUTE_WORK_GROUP_SIZE, wg));
            m_compute.local_size_x = static_cast<uint32>(wg[0]);
            m_compute.local_size_y = static_cast<uint32>(wg[1]);
            m_compute.local_size_z = static_cast<uint32>(wg[2]);
        }

        if (!success) {
            logger.error("Shader program reflection completed with errors. ");
        }

        m_valid = success;

        TAV_ASSERT(m_ubo_blocks.size() == m_ubo_ranges.size());
    }

    gl_shader_program_reflect::~gl_shader_program_reflect() noexcept
    {
    }

    bool gl_shader_program_reflect::is_valid() const noexcept
    {
        return m_valid;
    }

    core::buffer_view<vertex_attribute_reflect> gl_shader_program_reflect::vertex_attributes() const noexcept
    {
        return m_vert_attribs;
    }

    core::buffer_view<shader_resource_reflect> gl_shader_program_reflect::shader_resources() const noexcept
    {
        return m_shader_res;
    }

    core::buffer_view<constant_block_reflect> gl_shader_program_reflect::constant_blocks() const noexcept
    {
        return m_ubo_blocks;
    }

    core::buffer_view<member_reflect> gl_shader_program_reflect::constant_block_members(size_t constant_block_index) const noexcept
    {
        TAV_ASSERT(constant_block_index < m_ubo_ranges.size());

        if (constant_block_index >= m_ubo_ranges.size()) {
            return {};
        }

        auto range = m_ubo_ranges[constant_block_index];
        TAV_ASSERT(range.begin <= range.end);
        TAV_ASSERT(range.end <= m_ubo_members.size());

        if (range.begin >= range.end) {
            return {};
        }

        return {m_ubo_members.data() + range.begin, range.end - range.begin};
    }

    core::buffer_view<storage_block_reflect> gl_shader_program_reflect::storage_blocks() const noexcept
    {
        return m_ssbo_blocks;
    }

    core::buffer_view<output_reflect> gl_shader_program_reflect::outputs() const noexcept
    {
        return m_outputs;
    }

    const compute_reflect& gl_shader_program_reflect::compute() const noexcept
    {
        return m_compute;
    }

} // namespace tavros::renderer::rhi
