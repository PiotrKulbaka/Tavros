#include <stdio.h>

#include "render_app_base.hpp"
#include "built_in_meshes.hpp"
#include "fps_meter.hpp"

#include <tavros/renderer/shaders/shader_loader.hpp>

#include <tavros/core/memory/memory.hpp>
#include <tavros/core/memory/mallocator.hpp>
#include <tavros/core/memory/buffer.hpp>
#include <tavros/core/utf8.hpp>
#include <tavros/core/geometry/aabb2.hpp>
#include <tavros/core/exception.hpp>
#include <tavros/core/timer.hpp>

#include <tavros/renderer/rhi/command_queue.hpp>
#include <tavros/renderer/rhi/graphics_device.hpp>
#include <tavros/renderer/camera/camera.hpp>
#include <tavros/renderer/debug_renderer.hpp>
#include <tavros/renderer/render_target.hpp>
#include <tavros/renderer/gpu_stream_buffer.hpp>

#include <tavros/input/input_manager.hpp>

#include <tavros/resources/resource_manager.hpp>
#include <tavros/resources/providers/filesystem_provider.hpp>
#include <tavros/resources/codecs/image_codec.hpp>

#include <tavros/ui/view.hpp>
#include <tavros/ui/button/button.hpp>
#include <tavros/ui/root_view.hpp>

#include <tavros/text/font/font_library.hpp>
#include <tavros/text/text_layout.hpp>

#include <tavros/system/application.hpp>

#include <algorithm>

namespace rhi = tavros::renderer::rhi;

const tavros::core::string_view lorem_ipsum = R"(Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec commodo tellus eu urna fermentum, porta facilisis libero lobortis. Vestibulum est urna, fermentum sed fringilla sed, dignissim quis est. Donec tempus sed enim vitae aliquet. Nullam rhoncus, erat at eleifend consequat, dolor tellus aliquam orci, eget varius mi est ut turpis. In nec enim eu nisi lobortis euismod in a neque. Fusce interdum vel turpis sit amet sodales. Nam quis volutpat est. Duis interdum libero eget dui dapibus laoreet. Mauris condimentum elit turpis, a semper ex elementum eu. Nam finibus urna purus, sit amet pellentesque ante pharetra id. Vivamus nisl ligula, lacinia consequat lacinia id, scelerisque ac sem. Vivamus pellentesque, ligula in posuere sodales, sem ante condimentum purus, in laoreet velit quam ac ipsum. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Curabitur fermentum sit amet risus vitae aliquam. Aliquam id dapibus risus, et rhoncus tellus. Ut sit amet elementum leo.

Cras elementum lacinia diam nec eleifend. Nullam consequat porttitor tellus, ac finibus mauris suscipit non. Sed semper turpis non elit suscipit, et viverra risus suscipit. Aliquam sagittis felis et nibh consectetur facilisis eget quis nisl. Fusce blandit felis id nibh luctus ornare. Etiam cursus mattis velit, sed maximus massa tristique vel. Pellentesque vitae euismod augue. Duis vel velit eu nulla egestas convallis ac non eros. Proin quis sem quam. Phasellus elementum urna ut ipsum blandit vulputate. Duis lobortis rhoncus vestibulum.

Morbi ultrices, metus at viverra scelerisque, metus sem accumsan nisl, sit amet tincidunt risus nisi non nibh. Integer sed molestie ligula. Integer eu massa id sapien blandit sagittis vel in magna. Nam vestibulum massa nisl, vitae vestibulum mi molestie et. Fusce hendrerit augue ante, ut tincidunt leo commodo quis. Aenean blandit non urna at auctor. Sed luctus, metus nec dapibus consequat, nisi lacus fringilla arcu, eget posuere risus nunc in neque. Aenean lobortis nec diam ut lobortis. Phasellus bibendum tellus in sapien porta ultrices. Fusce nec ex iaculis, hendrerit sapien a, accumsan ante. Donec non tortor interdum, facilisis magna vitae, fermentum mi. Maecenas enim diam, fringilla a gravida sit amet, feugiat et lectus. Nunc dapibus ultrices est vitae ultrices. Mauris mattis nec nunc vel varius. Aliquam tincidunt ac ante sed elementum.

Aliquam vitae rutrum purus. Nullam finibus, augue luctus molestie lacinia, nibh lectus rutrum odio, ut dapibus massa massa nec augue. Aliquam ac leo pharetra, finibus arcu nec, fermentum nibh. Nullam elit lorem, vehicula eget mollis vitae, dictum a eros. Suspendisse ultricies elit placerat, cursus ipsum id, pharetra justo. Nam eu libero et enim pretium pulvinar nec vel felis. Maecenas rhoncus est lobortis nibh commodo fermentum. Suspendisse ultricies tellus vulputate purus feugiat imperdiet. Vestibulum efficitur rutrum turpis, id pretium metus interdum at. Vestibulum sed purus eget diam bibendum sodales. Etiam ac massa sit amet enim fermentum tempor eu vitae justo. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Quisque est erat, condimentum non suscipit a, bibendum eu nisi. Pellentesque dolor massa, gravida sed efficitur id, tincidunt vel lectus. Suspendisse laoreet libero in mi molestie, sit amet commodo lacus tristique. Mauris egestas ultricies cursus.

Integer ipsum est, tempus non mollis vel, blandit sit amet mauris. Duis luctus ligula vitae urna egestas, ut venenatis arcu malesuada. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Morbi in nisl sed erat laoreet elementum id ut mauris. Morbi faucibus nunc turpis, eu blandit nisi ullamcorper in. Cras malesuada maximus elit, sed porttitor ipsum dapibus elementum. Fusce feugiat metus risus, sit amet dapibus arcu eleifend vel.
)";

const tavros::core::string_view lorem_ipsum_2 = "  Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec commodo tellus eu urna fermentum, porta facilisis libero lobortis. Vestibulum est urna, fermentum sed fringilla sed, dignissim quis est. Donec tempus sed enim vitae aliquet. Nullam rhoncus, erat at eleifend consequat, dolor tellus aliquam orci, eget varius mi est ut turpis. In nec enim eu nisi lobortis euismod in a neque. Fusce interdum vel turpis sit amet sodales. Nam quis volutpat est. Duis interdum libero eget dui dapibus laoreet. Mauris condimentum elit turpis, a semper ex elementum eu. Nam finibus urna purus, sit amet pellentesque ante pharetra id. Vivamus nisl ligula, lacinia consequat lacinia id, scelerisque ac sem. Vivamus pellentesque, ligula in posuere sodales, sem ante condimentum purus, in laoreet velit quam ac ipsum. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Curabitur fermentum sit amet risus vitae aliquam. Aliquam id dapibus risus, et rhoncus tellus. Ut sit amet elementum leo. В частности, постоянный количественный рост и сфера нашей активности, а также свежий взгляд на привычные вещи — безусловно открывает новые горизонты для поставленных обществом задач. И нет сомнений, что действия представителей оппозиции неоднозначны и будут представлены в исключительно положительном свете. Также как выбранный нами инновационный путь способствует подготовке и реализации поэтапного и последовательного развития общества. Повседневная практика показывает, что курс на социально-ориентированный национальный проект, а также свежий взгляд на привычные вещи — безусловно открывает новые горизонты для инновационных методов управления процессами. В частности, глубокий уровень погружения позволяет оценить значение распределения внутренних резервов и ресурсов.";

namespace rhi = tavros::renderer::rhi;

static tavros::core::logger logger("main");

constexpr float k_sdf_size_pix = 8.0f;
constexpr float k_font_scale_pix = 96.0f;

[[noreturn]] void exit_fail()
{
    std::exit(-1);
}


struct glyph_params
{
    tavros::math::rgba8 fill_color;
    tavros::math::rgba8 outline_color;
};

struct glyph_instance
{
    float                    mat[3][2] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    tavros::text::atlas_rect rect;
    tavros::math::rgba8      fill_color;
    tavros::math::rgba8      outline_color;
};


struct mat32_component
{
    float mat[3][2] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
};

struct atlas_rect_component
{
    tavros::text::atlas_rect rect;
};

struct color_component
{
    tavros::math::rgba8 color;
};


template<class GlyphData>
size_t fill_glyphs_instances(tavros::core::buffer_span<GlyphData> glyphs, tavros::core::buffer_span<glyph_instance> buf, tavros::math::vec2 pos_text, float pad)
{
    TAV_ASSERT(buf.size() >= glyphs.size());

    size_t len = 0;
    auto*  dst = buf.begin();
    for (auto src = glyphs.begin(); src < glyphs.end(); ++src) {
        if (!src->base.is_space) {
            auto scale = src->base.glyph_size;
            auto scaled_pad = pad * scale;
            auto size = src->base.layout.size();

            dst->mat[0][0] = size.width + scaled_pad * 2.0f;
            dst->mat[0][1] = 0.0f;
            dst->mat[1][0] = 0.0f;
            dst->mat[1][1] = size.height + scaled_pad * 2.0f;
            dst->mat[2][0] = pos_text.x + src->base.layout.min.x - scaled_pad;
            dst->mat[2][1] = pos_text.y + src->base.layout.min.y - scaled_pad;

            dst->rect = src->base.rect;
            dst->fill_color = src->params.fill_color;
            dst->outline_color = src->params.outline_color;

            ++dst;
            ++len;
        }
    }

    return len;
}

class my_shader_provider : public tavros::renderer::shader_source_provider
{
public:
    my_shader_provider(tavros::core::shared_ptr<tavros::resources::resource_manager> rm)
        : m_rm(rm)
    {
    }

    ~my_shader_provider() noexcept override = default;

    tavros::core::string load(tavros::core::string_view path) override
    {
        return m_rm->open(path)->reader()->read_as_text();
    }

private:
    tavros::core::shared_ptr<tavros::resources::resource_manager> m_rm;
};

class my_font_data_provider : public tavros::text::font_data_provider
{
public:
    my_font_data_provider(tavros::core::shared_ptr<tavros::resources::resource_manager> rm)
        : m_rm(rm)
    {
    }

    ~my_font_data_provider() noexcept override = default;

    tavros::core::vector<uint8> load(tavros::core::string_view path) override
    {
        return m_rm->open(path)->reader()->read_as_binary();
    }

private:
    tavros::core::shared_ptr<tavros::resources::resource_manager> m_rm;
};

class main_window : public app::render_app_base
{
public:
    main_window(tavros::core::string_view name, tavros::core::shared_ptr<tavros::resources::resource_manager> resource_manager)
        : app::render_app_base(name)
        , m_imcodec(&m_allocator)
        , m_resource_manager(resource_manager)
        , m_sl(std::move(tavros::core::make_unique<my_shader_provider>(resource_manager)))
        , m_font_lib(std::move(tavros::core::make_unique<my_font_data_provider>(resource_manager)))
    {
    }

    ~main_window() override
    {
    }

    rhi::buffer_handle create_stage_buffer(size_t size, rhi::buffer_access access)
    {
        rhi::buffer_create_info info;
        info.size = size;
        info.usage = rhi::buffer_usage::stage;
        info.access = access;
        auto buffer = m_graphics_device->create_buffer(info);
        if (!buffer) {
            ::logger.fatal("Failed to create stage buffer.");
            exit_fail();
        }
        return buffer;
    }

    tavros::resources::image_codec::pixels_view load_image(tavros::core::string_view path, bool y_flip = false)
    {
        tavros::core::dynamic_buffer<uint8> buffer(&m_allocator);

        try {
            auto reader = m_resource_manager->open(path, tavros::resources::resource_access::read_only)->reader();
            buffer.reserve(reader->size());
            reader->read(buffer);
        } catch (tavros::core::file_error& e) {
            ::logger.error("Failed to open image '{}'", e.path());
        }

        // buffer.data() can be nullptr; decode_image will return fallback with white pixel
        return m_imcodec.decode(buffer, 4, y_flip);
    }

    bool save_image(const tavros::resources::image_codec::pixels_view& pixels, tavros::core::string_view path, bool y_flip = false)
    {
        auto im_data = m_imcodec.encode(pixels, tavros::resources::image_codec::image_format::png, y_flip);
        if (im_data.empty()) {
            ::logger.error("Failed to save image '{}': im_data is empty()", path);
            return false;
        }

        try {
            m_resource_manager->open(path, tavros::resources::resource_access::write_only)->writer()->write(im_data);
            return true;
        } catch (const tavros::core::file_error& e) {
            ::logger.error("Failed to save image: '{}'", e.path());
        }

        return false;
    }

    rhi::shader_binding_handle upload_font_data()
    {
        auto new_atlas_pix = m_font_lib.invalidate_old_and_bake_new_atlas(k_font_scale_pix, k_sdf_size_pix);

        size_t                                      new_atlas_im_size = new_atlas_pix.width * new_atlas_pix.height;
        tavros::resources::image_codec::pixels_view new_atlas_im{new_atlas_pix.width, new_atlas_pix.height, 1, new_atlas_pix.width, {new_atlas_pix.pixels.data(), new_atlas_im_size}};
        // save_image(new_atlas_im, "font_atlas_new.png");

        // create texture
        size_t tex_size = new_atlas_im.width * new_atlas_im.height;
        auto   dst = m_graphics_device->map_buffer(m_stage_buffer);
        dst.copy_from(new_atlas_pix.pixels.data(), tex_size);
        m_graphics_device->unmap_buffer(m_stage_buffer);

        rhi::texture_create_info tex_info;
        tex_info.type = rhi::texture_type::texture_2d;
        tex_info.format = rhi::pixel_format::r8un;
        tex_info.width = new_atlas_im.width;
        tex_info.height = new_atlas_im.height;
        tex_info.depth = 1;
        tex_info.usage = rhi::k_default_texture_usage;
        tex_info.mip_levels = 1;
        tex_info.array_layers = 1;
        tex_info.sample_count = 1;

        auto texture = m_graphics_device->create_texture(tex_info);
        if (!texture) {
            ::logger.fatal("Failed to create sdf texture");
            exit_fail();
        }

        auto* cbuf = m_composer->create_command_queue();
        cbuf->wait_for_fence(m_fence);
        rhi::texture_copy_region rgn;
        rgn.width = new_atlas_im.width;
        rgn.height = new_atlas_im.height;
        cbuf->copy_buffer_to_texture(m_stage_buffer, texture, rgn);
        cbuf->signal_fence(m_fence);
        m_composer->submit_command_queue(cbuf);

        rhi::sampler_create_info sampler_info;
        sampler_info.filter.mipmap_filter = rhi::mipmap_filter_mode::off;
        sampler_info.filter.min_filter = rhi::filter_mode::linear;
        sampler_info.filter.mag_filter = rhi::filter_mode::linear;

        auto sampler = m_graphics_device->create_sampler(sampler_info);
        if (!sampler) {
            ::logger.fatal("Failed to create sdf sampler");
            exit_fail();
        }

        rhi::shader_binding_create_info shader_binding_info;
        shader_binding_info.texture_bindings.push_back({texture, sampler, 0});
        auto sh_binding = m_graphics_device->create_shader_binding(shader_binding_info);
        if (!sh_binding) {
            ::logger.fatal("Failed to create shader binding pass");
            exit_fail();
        }

        return sh_binding;
    }

    tavros::core::string load_shader(tavros::core::string_view path)
    {
        return m_sl.load(path, {tavros::renderer::shader_language::glsl_460, ""});
    }

    void init() override
    {
        m_camera.set_orientation({1.0f, 1.0f, -0.25f}, {0.0f, 0.0f, 1.0f});
        m_camera.set_position({-8.0f, -8.0f, 5.0f});

        m_graphics_device = rhi::graphics_device::create(rhi::render_backend_type::opengl);
        if (!m_graphics_device) {
            ::logger.fatal("Failed to create graphics_device.");
            exit_fail();
        }

        rhi::frame_composer_create_info main_composer_info;
        main_composer_info.width = 1;
        main_composer_info.height = 1;
        main_composer_info.buffer_count = 3;
        main_composer_info.vsync = true;
        main_composer_info.color_attachment_format = rhi::pixel_format::rgba8un;
        main_composer_info.depth_stencil_attachment_format = rhi::pixel_format::depth32f_stencil8;
        main_composer_info.native_handle = native_handle();

        auto main_composer_handle = m_graphics_device->create_frame_composer(main_composer_info);
        if (!main_composer_handle) {
            ::logger.fatal("Failed to create main frame composer.");
            exit_fail();
        }

        m_composer = m_graphics_device->get_frame_composer_ptr(main_composer_handle);
        if (m_composer == nullptr) {
            ::logger.fatal("Failed to get main frame composer.");
            exit_fail();
        }


        m_stream_draw = tavros::core::make_unique<tavros::renderer::gpu_stream_buffer>(m_graphics_device.get(), 1024ull * 1024ull * 128ull, rhi::buffer_usage::vertex);


        m_offscreen_rt = tavros::core::make_unique<tavros::renderer::render_target>(rhi::pixel_format::rgba8un, rhi::pixel_format::depth32f);
        m_offscreen_rt->init(m_graphics_device.get());

        auto fullscreen_quad_vertex_shader_source = load_shader("tavros/shaders/fullscreen_quad.vert");
        auto fullscreen_quad_vertex_shader = m_graphics_device->create_shader({fullscreen_quad_vertex_shader_source, rhi::shader_stage::vertex, "main"});
        auto fullscreen_quad_fragment_shader_source = load_shader("tavros/shaders/fullscreen_quad.frag");
        auto fullscreen_quad_fragment_shader = m_graphics_device->create_shader({fullscreen_quad_fragment_shader_source, rhi::shader_stage::fragment, "main"});

        rhi::pipeline_create_info fullscreen_quad_pipeline_info;
        fullscreen_quad_pipeline_info.shaders.push_back(fullscreen_quad_vertex_shader);
        fullscreen_quad_pipeline_info.shaders.push_back(fullscreen_quad_fragment_shader);
        fullscreen_quad_pipeline_info.depth_stencil.depth_test_enable = false;
        fullscreen_quad_pipeline_info.depth_stencil.depth_write_enable = false;
        fullscreen_quad_pipeline_info.depth_stencil.depth_compare = rhi::compare_op::less;
        fullscreen_quad_pipeline_info.rasterizer.cull = rhi::cull_face::off;
        fullscreen_quad_pipeline_info.rasterizer.polygon = rhi::polygon_mode::fill;
        fullscreen_quad_pipeline_info.topology = rhi::primitive_topology::triangle_strip;
        fullscreen_quad_pipeline_info.blend_states.push_back({false, rhi::blend_factor::src_alpha, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::blend_factor::one, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::k_rgba_color_mask});

        m_fullscreen_quad_pipeline = m_graphics_device->create_pipeline(fullscreen_quad_pipeline_info);

        m_graphics_device->destroy_shader(fullscreen_quad_vertex_shader);
        m_graphics_device->destroy_shader(fullscreen_quad_fragment_shader);

        m_stage_buffer = create_stage_buffer(1024 * 1024 * 128, rhi::buffer_access::cpu_to_gpu);

        m_stage_upload_buffer = create_stage_buffer(1024 * 1024 * 64, rhi::buffer_access::gpu_to_cpu);

        m_fence = m_graphics_device->create_fence();
        if (!m_fence) {
            ::logger.fatal("Failed to create fence");
            exit_fail();
        }

        auto im_view = load_image("textures/cube_test.png");

        size_t tex_size = im_view.width * im_view.height * im_view.channels;
        auto   dst = m_graphics_device->map_buffer(m_stage_buffer);
        dst.copy_from(im_view.pixels.data(), tex_size);
        m_graphics_device->unmap_buffer(m_stage_buffer);

        rhi::texture_create_info tex_create_info;
        tex_create_info.type = rhi::texture_type::texture_2d;
        tex_create_info.format = rhi::pixel_format::rgba8un;
        tex_create_info.width = im_view.width;
        tex_create_info.height = im_view.height;
        tex_create_info.depth = 1;
        tex_create_info.usage = rhi::k_default_texture_usage;
        tex_create_info.mip_levels = tavros::math::mip_levels(im_view.width, im_view.height);
        tex_create_info.array_layers = 1;
        tex_create_info.sample_count = 1;

        m_texture = m_graphics_device->create_texture(tex_create_info);
        if (!m_texture) {
            ::logger.fatal("Failed to create texture");
            exit_fail();
        }


        rhi::texture_copy_region copy_region;

        copy_region.width = im_view.width;
        copy_region.height = im_view.height;
        copy_region.buffer_row_length = im_view.stride / im_view.channels;

        auto* cbuf = m_composer->create_command_queue();
        cbuf->copy_buffer_to_texture(m_stage_buffer, m_texture, copy_region);
        cbuf->signal_fence(m_fence);
        m_composer->submit_command_queue(cbuf);

        rhi::sampler_create_info sampler_info;
        sampler_info.filter.mipmap_filter = rhi::mipmap_filter_mode::off;
        sampler_info.filter.min_filter = rhi::filter_mode::linear;
        sampler_info.filter.mag_filter = rhi::filter_mode::linear;

        m_sampler = m_graphics_device->create_sampler(sampler_info);
        if (!m_sampler) {
            ::logger.fatal("Failed to create sampler");
            exit_fail();
        }

        rhi::render_pass_create_info main_render_pass;
        main_render_pass.color_attachments.push_back({rhi::pixel_format::rgba8un, rhi::load_op::clear, rhi::store_op::dont_care, {}, {0.2f, 0.2f, 0.25f, 1.0f}});
        main_render_pass.depth_stencil_attachment.format = rhi::pixel_format::depth32f_stencil8;
        main_render_pass.depth_stencil_attachment.depth_load = rhi::load_op::clear;
        main_render_pass.depth_stencil_attachment.depth_store = rhi::store_op::store;
        main_render_pass.depth_stencil_attachment.depth_clear_value = 1.0f;
        main_render_pass.depth_stencil_attachment.stencil_load = rhi::load_op::clear;
        main_render_pass.depth_stencil_attachment.stencil_store = rhi::store_op::dont_care;
        main_render_pass.depth_stencil_attachment.stencil_clear_value = 0;
        m_main_pass = m_graphics_device->create_render_pass(main_render_pass);
        if (!m_main_pass) {
            ::logger.fatal("Failed to create render pass");
            exit_fail();
        }

        rhi::shader_binding_create_info shader_binding_info;
        shader_binding_info.texture_bindings.push_back({m_texture, m_sampler, 0});
        m_shader_binding = m_graphics_device->create_shader_binding(shader_binding_info);
        if (!m_shader_binding) {
            ::logger.fatal("Failed to create render pass");
            exit_fail();
        }

        rhi::buffer_create_info uniform_buffer_desc{1024ull * 1024ull, rhi::buffer_usage::storage, rhi::buffer_access::gpu_only};
        m_uniform_buffer = m_graphics_device->create_buffer(uniform_buffer_desc);
        if (!m_uniform_buffer) {
            ::logger.fatal("Failed to create uniform buffer");
            exit_fail();
        }

        auto mesh_renderer_vertex_shader_source = load_shader("tavros/shaders/cube.vert");
        auto mesh_rendering_vertex_shader = m_graphics_device->create_shader({mesh_renderer_vertex_shader_source, rhi::shader_stage::vertex, "main"});
        auto mesh_renderer_fragment_shader_source = load_shader("tavros/shaders/cube.frag");
        auto mesh_rendering_fragment_shader = m_graphics_device->create_shader({mesh_renderer_fragment_shader_source, rhi::shader_stage::fragment, "main"});

        rhi::pipeline_create_info mesh_rendering_pipeline_info;

        mesh_rendering_pipeline_info.bindings.push_back({rhi::attribute_type::vec3, rhi::attribute_format::f32, false, 0, sizeof(app::vertex_type), offsetof(app::vertex_type, app::vertex_type::pos), 0});
        mesh_rendering_pipeline_info.bindings.push_back({rhi::attribute_type::vec3, rhi::attribute_format::f32, false, 1, sizeof(app::vertex_type), offsetof(app::vertex_type, app::vertex_type::normal), 0});
        mesh_rendering_pipeline_info.bindings.push_back({rhi::attribute_type::vec2, rhi::attribute_format::f32, false, 2, sizeof(app::vertex_type), offsetof(app::vertex_type, app::vertex_type::uv), 0});

        mesh_rendering_pipeline_info.shaders.push_back(mesh_rendering_vertex_shader);
        mesh_rendering_pipeline_info.shaders.push_back(mesh_rendering_fragment_shader);
        mesh_rendering_pipeline_info.depth_stencil.depth_test_enable = true;
        mesh_rendering_pipeline_info.depth_stencil.depth_write_enable = true;
        mesh_rendering_pipeline_info.depth_stencil.depth_compare = rhi::compare_op::less;
        mesh_rendering_pipeline_info.rasterizer.cull = rhi::cull_face::back;
        mesh_rendering_pipeline_info.rasterizer.face = rhi::front_face::counter_clockwise;
        mesh_rendering_pipeline_info.rasterizer.polygon = rhi::polygon_mode::fill;
        mesh_rendering_pipeline_info.topology = rhi::primitive_topology::triangles;
        mesh_rendering_pipeline_info.blend_states.push_back({false, rhi::blend_factor::src_alpha, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::blend_factor::one, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::k_rgba_color_mask});
        mesh_rendering_pipeline_info.multisample.sample_shading_enabled = false;
        mesh_rendering_pipeline_info.multisample.sample_count = 1;
        mesh_rendering_pipeline_info.multisample.min_sample_shading = 0.0;

        m_mesh_rendering_pipeline = m_graphics_device->create_pipeline(mesh_rendering_pipeline_info);
        if (!m_mesh_rendering_pipeline) {
            ::logger.fatal("Failed to create mesh rendering pipeline");
            exit_fail();
        }

        m_graphics_device->destroy_shader(mesh_rendering_vertex_shader);
        m_graphics_device->destroy_shader(mesh_rendering_fragment_shader);


        rhi::buffer_create_info mesh_vertices_buffer_info{1024 * 1024 * 16, rhi::buffer_usage::vertex, rhi::buffer_access::cpu_to_gpu};
        m_mesh_vertices_buffer = m_graphics_device->create_buffer(mesh_vertices_buffer_info);
        if (!m_mesh_vertices_buffer) {
            ::logger.fatal("Failed to create mesh vertices buffer");
            exit_fail();
        }

        rhi::buffer_create_info mesh_indices_buffer_info{1024 * 128, rhi::buffer_usage::index, rhi::buffer_access::cpu_to_gpu};
        m_mesh_indices_buffer = m_graphics_device->create_buffer(mesh_indices_buffer_info);
        if (!m_mesh_indices_buffer) {
            ::logger.fatal("Failed to create mesh indices buffer");
            exit_fail();
        }

        auto verts_map = m_graphics_device->map_buffer(m_mesh_vertices_buffer);
        verts_map.copy_from(app::cube_vertices, sizeof(app::cube_vertices));
        m_graphics_device->unmap_buffer(m_mesh_vertices_buffer);

        auto inds_map = m_graphics_device->map_buffer(m_mesh_indices_buffer);
        inds_map.copy_from(app::cube_indices, sizeof(app::cube_indices));
        m_graphics_device->unmap_buffer(m_mesh_indices_buffer);

        rhi::shader_binding_create_info mesh_shader_binding_info;
        mesh_shader_binding_info.texture_bindings.push_back({m_texture, m_sampler, 0});

        m_mesh_shader_binding = m_graphics_device->create_shader_binding(mesh_shader_binding_info);
        if (!m_mesh_shader_binding) {
            ::logger.fatal("Failed to create shader binding");
            exit_fail();
        }


        // World grid
        auto world_grid_vertex_shader_source = load_shader("tavros/shaders/world_grid.vert");
        auto world_grid_rendering_vertex_shader = m_graphics_device->create_shader({world_grid_vertex_shader_source, rhi::shader_stage::vertex, "main"});
        auto world_grid_fragment_shader_source = load_shader("tavros/shaders/world_grid.frag");
        auto world_grid_rendering_fragment_shader = m_graphics_device->create_shader({world_grid_fragment_shader_source, rhi::shader_stage::fragment, "main"});

        rhi::pipeline_create_info world_grid_rendering_pipeline_info;
        world_grid_rendering_pipeline_info.shaders.push_back(world_grid_rendering_vertex_shader);
        world_grid_rendering_pipeline_info.shaders.push_back(world_grid_rendering_fragment_shader);
        world_grid_rendering_pipeline_info.depth_stencil.depth_test_enable = true;
        world_grid_rendering_pipeline_info.depth_stencil.depth_write_enable = false;
        world_grid_rendering_pipeline_info.depth_stencil.depth_compare = rhi::compare_op::less;
        world_grid_rendering_pipeline_info.rasterizer.cull = rhi::cull_face::off;
        world_grid_rendering_pipeline_info.rasterizer.face = rhi::front_face::counter_clockwise;
        world_grid_rendering_pipeline_info.rasterizer.polygon = rhi::polygon_mode::fill;
        world_grid_rendering_pipeline_info.topology = rhi::primitive_topology::triangle_strip;
        world_grid_rendering_pipeline_info.blend_states.push_back({true, rhi::blend_factor::src_alpha, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::blend_factor::one, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::k_rgba_color_mask});
        world_grid_rendering_pipeline_info.multisample.sample_shading_enabled = false;
        world_grid_rendering_pipeline_info.multisample.sample_count = 1;
        world_grid_rendering_pipeline_info.multisample.min_sample_shading = 0.0;

        m_world_grid_rendering_pipeline = m_graphics_device->create_pipeline(world_grid_rendering_pipeline_info);
        if (!m_world_grid_rendering_pipeline) {
            ::logger.fatal("Failed to create world grid rendering pipeline");
            exit_fail();
        }

        m_graphics_device->destroy_shader(world_grid_rendering_vertex_shader);
        m_graphics_device->destroy_shader(world_grid_rendering_fragment_shader);


        rhi::shader_binding_create_info scene_shader_binding_info;
        scene_shader_binding_info.buffer_bindings.push_back({m_uniform_buffer, 0, sizeof(frame_data), 0});

        m_scene_binding = m_graphics_device->create_shader_binding(scene_shader_binding_info);
        if (!m_scene_binding) {
            ::logger.fatal("Failed to create world grid shader binding");
            exit_fail();
        }

        m_graphics_device->wait_for_fence(m_fence);

        show();

        m_font_lib.load("fonts/Consola-Mono.ttf", "Consola-Mono");
        m_font_lib.load("fonts/DroidSans.ttf", "DroidSans");
        m_font_lib.load("fonts/HomeVideo.ttf", "HomeVideo");
        m_font_lib.load("fonts/NotoSans-Regular.ttf", "NotoSans-Regular");
        m_font_lib.load("fonts/Roboto-Medium.ttf", "Roboto-Medium");

        m_font_shader_binding = upload_font_data();

        // SDF
        auto sdf_font_vertex_shader_source = load_shader("tavros/shaders/sdf_font.vert");
        auto sdf_font_vertex_shader = m_graphics_device->create_shader({sdf_font_vertex_shader_source, rhi::shader_stage::vertex, "main"});
        auto sdf_font_fragment_shader_source = load_shader("tavros/shaders/sdf_font.frag");
        auto sdf_font_fragment_shader = m_graphics_device->create_shader({sdf_font_fragment_shader_source, rhi::shader_stage::fragment, "main"});

        rhi::pipeline_create_info sdf_font_pipeline_info;


        sdf_font_pipeline_info.bindings.push_back({rhi::attribute_type::mat3x2, rhi::attribute_format::f32, false, 0, sizeof(glyph_instance), offsetof(glyph_instance, glyph_instance::mat), 1});
        sdf_font_pipeline_info.bindings.push_back({rhi::attribute_type::vec2, rhi::attribute_format::u32, false, 3, sizeof(glyph_instance), offsetof(glyph_instance, glyph_instance::rect), 1});
        sdf_font_pipeline_info.bindings.push_back({rhi::attribute_type::scalar, rhi::attribute_format::u32, false, 4, sizeof(glyph_instance), offsetof(glyph_instance, glyph_instance::fill_color), 1});
        sdf_font_pipeline_info.bindings.push_back({rhi::attribute_type::scalar, rhi::attribute_format::u32, false, 5, sizeof(glyph_instance), offsetof(glyph_instance, glyph_instance::outline_color), 1});

        sdf_font_pipeline_info.shaders.push_back(sdf_font_vertex_shader);
        sdf_font_pipeline_info.shaders.push_back(sdf_font_fragment_shader);
        sdf_font_pipeline_info.depth_stencil.depth_test_enable = false;
        sdf_font_pipeline_info.rasterizer.cull = rhi::cull_face::off;
        sdf_font_pipeline_info.rasterizer.face = rhi::front_face::counter_clockwise;
        sdf_font_pipeline_info.rasterizer.polygon = rhi::polygon_mode::fill;
        sdf_font_pipeline_info.rasterizer.scissor_enable = true;
        sdf_font_pipeline_info.topology = rhi::primitive_topology::triangle_strip;
        sdf_font_pipeline_info.blend_states.push_back({true, rhi::blend_factor::src_alpha, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::blend_factor::one, rhi::blend_factor::one_minus_src_alpha, rhi::blend_op::add, rhi::k_rgba_color_mask});
        sdf_font_pipeline_info.multisample.sample_shading_enabled = false;
        sdf_font_pipeline_info.multisample.sample_count = 1;
        sdf_font_pipeline_info.multisample.min_sample_shading = 0.0;

        m_sdf_font_pipeline = m_graphics_device->create_pipeline(sdf_font_pipeline_info);
        if (!m_sdf_font_pipeline) {
            ::logger.fatal("Failed to create sdf font rendering pipeline");
            exit_fail();
        }

        m_drenderer.init(m_graphics_device.get());

        m_root_view.init(m_graphics_device.get());

        m_root_view.root().insert_last_child(&m_view1);
        m_root_view.root().insert_last_child(&m_view2);
        m_root_view.root().insert_last_child(&m_view3);
        m_root_view.root().insert_last_child(&m_view4);
        m_root_view.root().insert_last_child(&m_view5);
        m_root_view.root().insert_last_child(&m_btn0);

        m_view5.insert_last_child(&m_view6);
        m_view5.insert_last_child(&m_view7);
        m_view5.insert_last_child(&m_view8);
        m_view5.insert_last_child(&m_view9);
        m_view5.insert_last_child(&m_btn1);
        m_view5.set_enabled(false);

        m_btn0.set_position({1400, 100});
        m_btn0.set_size({240.0f, 80.0f});
        m_btn0.set_text("Button0");

        m_btn1.set_position({300, 100});
        m_btn1.set_size({240.0f, 80.0f});
        m_btn1.set_text("Button1");

        m_view9.insert_last_child(&m_view10);
        m_view9.insert_last_child(&m_view11);

        m_view1.set_position({100, 100});
        m_view1.set_size({100, 100});

        m_view2.set_position({100, 300});
        m_view2.set_size({100, 100});

        m_view3.set_position({300, 100});
        m_view3.set_size({150, 150});

        m_view4.set_position({300, 300});
        m_view4.set_size({200, 100});

        m_view5.set_position({1250, 100});
        m_view5.set_size({600, 600});
        m_view5.set_padding({10.0f, 20.0f, 30.0f, 40.0f});

        m_view6.set_position({10, 10});
        m_view6.set_size({100, 100});

        m_view7.set_position({250, 50});
        m_view7.set_size({100, 150});
        m_view7.set_padding({10.0f, 10.0f, 10.0f, 10.0f});

        m_view8.set_position({50, 250});
        m_view8.set_size({100, 150});

        m_view9.set_position({250, 250});
        m_view9.set_size({250, 300});

        m_view10.set_position({50, 50});
        m_view10.set_size({100, 50});

        m_view11.set_position({50, 150});
        m_view11.set_size({150, 100});

        m_rich_line.set_text(lorem_ipsum_2, m_font_lib.fonts()[0].font.get(), 100.0f);

        for (auto& it : m_rich_line.glyphs()) {
            it.params.fill_color.set(255, 255, 255, 255);
            it.params.outline_color.set(128, 128, 255, 0);
        }
    }

    void shutdown() override
    {
        m_stream_draw = nullptr;
        m_drenderer.shutdown();
        m_root_view.shutdown();
        m_offscreen_rt->shutdown();
        m_graphics_device = nullptr;
    }

    void process_events(tavros::input::event_args_queue_view events, double delta_time)
    {
        m_root_view.process_ui_events(events);

        m_input_manager.begin_frame(tavros::system::application::instance().highp_time_us());
        m_input_manager.process_events(events);

        if (m_input_manager.is_key_up(tavros::input::keyboard_key::k_F1)) {
            if (m_current_buffer_output_index != 0) {
                m_current_buffer_output_index = 0;
            }
        } else if (m_input_manager.is_key_up(tavros::input::keyboard_key::k_F2)) {
            if (m_current_buffer_output_index != 1) {
                m_current_buffer_output_index = 1;
            }
        }

        if (m_input_manager.is_key_pressed(tavros::input::keyboard_key::k_minus)) {
            m_font_height -= 0.2f;
        }

        if (m_input_manager.is_key_pressed(tavros::input::keyboard_key::k_equal)) {
            m_font_height += 0.2f;
        }


        if (m_input_manager.is_window_resized()) {
            auto sz = m_input_manager.get_window_size();
            if (sz.width != 0 && sz.height != 0) {
                m_offscreen_rt->recreate(static_cast<uint32>(sz.width), static_cast<uint32>(sz.height), 32);

                if (m_color_shader_binding) {
                    m_graphics_device->destroy_shader_binding(m_color_shader_binding);
                    m_color_shader_binding = rhi::shader_binding_handle();
                }

                if (m_depth_stencil_shader_binding) {
                    m_graphics_device->destroy_shader_binding(m_depth_stencil_shader_binding);
                    m_depth_stencil_shader_binding = rhi::shader_binding_handle();
                }

                rhi::shader_binding_create_info color_quad_shader_binding_info;
                color_quad_shader_binding_info.texture_bindings.push_back({m_offscreen_rt->get_color_attachment(0), m_sampler, 0});
                m_color_shader_binding = m_graphics_device->create_shader_binding(color_quad_shader_binding_info);
                if (!m_color_shader_binding) {
                    ::logger.fatal("Failed to create color quad shader binding");
                    exit_fail();
                }

                rhi::shader_binding_create_info depth_stencil_quad_shader_binding_info;
                depth_stencil_quad_shader_binding_info.texture_bindings.push_back({m_offscreen_rt->get_depth_stencil_attachment(), m_sampler, 0});
                m_depth_stencil_shader_binding = m_graphics_device->create_shader_binding(depth_stencil_quad_shader_binding_info);
                if (!m_depth_stencil_shader_binding) {
                    ::logger.fatal("Failed to create depth/stencil quad shader binding");
                    exit_fail();
                }

                m_composer->resize(sz.width, sz.height);

                constexpr float fov_y = 60.0f * 3.14159265358979f / 180.0f; // 60 deg
                float           aspect_ratio = static_cast<float>(sz.width) / static_cast<float>(sz.height);
                m_camera.set_perspective(fov_y, aspect_ratio, 0.1f, 1000.0f);
            }
        }

        // Update camera
        auto factor = [&](tavros::input::keyboard_key key) -> float {
            return static_cast<float>(m_input_manager.key_hold_factor(key));
        };

        // clang-format off
        tavros::math::vec3 move_delta =
            m_camera.forward() * factor(tavros::input::keyboard_key::k_W)
            - m_camera.forward() * factor(tavros::input::keyboard_key::k_S)
            + m_camera.right() * factor(tavros::input::keyboard_key::k_A)
            - m_camera.right() * factor(tavros::input::keyboard_key::k_D) 
            + m_camera.up() * factor(tavros::input::keyboard_key::k_space) 
            - m_camera.up() * factor(tavros::input::keyboard_key::k_C);
        // clang-format on

        float len = tavros::math::length(move_delta);
        if (len > 1.0f) {
            move_delta /= len;
        }

        bool  is_shift_pressed = m_input_manager.is_key_pressed(tavros::input::keyboard_key::k_lshift);
        bool  is_control_pressed = m_input_manager.is_key_pressed(tavros::input::keyboard_key::k_lcontrol);
        float speed_factor = static_cast<float>(delta_time) * (is_shift_pressed ? 10.0f * (is_control_pressed ? 10.0f : 1.0f) : 2.0f);
        m_camera.move(move_delta * speed_factor);

        // Update camera rotation
        auto mouse_delta = m_input_manager.get_smooth_mouse_delta();

        if (tavros::math::squared_length(mouse_delta) > 0.0f) {
            constexpr float base_sensitivity = 0.5f;
            auto            scaled_mouse_delta = mouse_delta * base_sensitivity;

            auto world_up = m_camera.world_up();

            auto q_yaw = tavros::math::quat::from_axis_angle(world_up, scaled_mouse_delta.x);
            auto q_pitch = tavros::math::quat::from_axis_angle(m_camera.right(), -scaled_mouse_delta.y);
            auto yaw_pitch_rotation = tavros::math::normalize(q_yaw * q_pitch);

            auto candidate_forward = tavros::math::normalize(yaw_pitch_rotation * m_camera.forward());

            auto current_forward_dot_up = tavros::math::dot(m_camera.forward(), world_up);
            auto forward_in_horizontal_plane = tavros::math::normalize(tavros::math::cross(m_camera.right(), world_up));
            auto candidate_dot_horizontal_forward = tavros::math::dot(candidate_forward, forward_in_horizontal_plane);

            if (candidate_dot_horizontal_forward < 0.000001f) {
                constexpr float cos_threshold = 0.000001f;
                auto            constrained_forward = forward_in_horizontal_plane * cos_threshold + (current_forward_dot_up > 0 ? world_up : -world_up);
                m_camera.set_orientation(q_yaw * constrained_forward, world_up);
                m_camera.rotate(q_yaw);
            } else {
                m_camera.set_orientation(candidate_forward, world_up);
            }
        }
    }

    void update_scene()
    {
        auto w = static_cast<float>(m_input_manager.get_window_size().width);
        auto h = static_cast<float>(m_input_manager.get_window_size().height);
        m_renderer_frame_data.view = m_camera.get_view_matrix();
        m_renderer_frame_data.perspective_projection = m_camera.get_projection_matrix();
        m_renderer_frame_data.view_projection = m_camera.get_view_projection_matrix();
        m_renderer_frame_data.inverse_view = tavros::math::inverse(m_camera.get_view_matrix());
        m_renderer_frame_data.inverse_projection = tavros::math::inverse(m_camera.get_projection_matrix());
        m_renderer_frame_data.orto_projection = tavros::math::mat4::ortho(0.0f, w, h, 0.0f, 1.0f, -1.0f);
        m_renderer_frame_data.frame_width = w;
        m_renderer_frame_data.frame_height = h;
        m_renderer_frame_data.near_plane = m_camera.near_plane();
        m_renderer_frame_data.far_plane = m_camera.far_plane();
        m_renderer_frame_data.view_space_depth = m_camera.far_plane() - m_camera.near_plane();
        m_renderer_frame_data.aspect_ratio = m_camera.aspect();
        m_renderer_frame_data.fov_y = m_camera.fov_y();
    }

    void render(tavros::input::event_args_queue_view events, double delta_time) override
    {
        tavros::core::timer frame_timer;

        process_events(events, delta_time);
        m_fps_meter.tick(delta_time);

        update_scene();

        m_stream_draw->begin_frame();
        m_frame_number++;

        m_drenderer.begin_frame(m_renderer_frame_data.orto_projection, m_renderer_frame_data.view_projection);

        auto w = static_cast<float>(m_input_manager.get_window_size().width);
        auto h = static_cast<float>(m_input_manager.get_window_size().height);

        m_drenderer.point3d({1.0f, 1.0f, 1.0f}, 24.0f, {1.0f, 0.0f, 0.0f, 1.0f});

        m_drenderer.line3d({1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {2.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 0.0f});
        m_drenderer.line3d({1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 2.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 0.0f});
        m_drenderer.line3d({1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 2.0f}, {1.0f, 1.0f, 1.0f, 0.0f});

        m_drenderer.box3d(
            tavros::geometry::aabb3{
                {1.0f, 1.0f, 0.0f},
                {2.5f, 2.5f, 1.5f}
            },
            {0.0f, 1.0f, 0.0f, 0.5f} // зелёный с лёгкой прозрачностью
        );

        m_drenderer.box3d(
            tavros::geometry::aabb3{
                {1.0f, 1.0f, 0.0f},
                {2.5f, 2.5f, 1.5f}
            },
            {0.0f, 1.0f, 0.0f, 1.0f}, // зелёный с лёгкой прозрачностью
            tavros::renderer::debug_renderer::draw_mode::edges
        );

        // Куб, вытянутый вдоль оси Z
        m_drenderer.box3d(
            tavros::geometry::aabb3{
                {-2.0f, -1.0f, 0.0f},
                {-1.0f, 0.0f, 3.0f}
            },
            {0.5f, 0.5f, 1.0f, 0.5f} // голубоватый
        );

        m_drenderer.box3d(
            tavros::geometry::aabb3{
                {-2.0f, -1.0f, 0.0f},
                {-1.0f, 0.0f, 3.0f}
            },
            {0.5f, 0.5f, 1.0f, 1.0f}, // голубоватый
            tavros::renderer::debug_renderer::draw_mode::points
        );

        m_drenderer.box3d(
            tavros::geometry::aabb3{
                {-2.0f, -1.0f, 0.0f},
                {-1.0f, 0.0f, 3.0f}
            },
            {0.5f, 0.5f, 1.0f, 0.8f}, // голубоватый
            tavros::renderer::debug_renderer::draw_mode::edges
        );

        auto obb1 = tavros::geometry::obb3(
            {6.0f, 4.0f, 3.0f},
            tavros::math::normalize(tavros::math::vec3(0.5f, 0.0f, 0.5f)), // forward (наклон 30° по Z)
            tavros::math::normalize(tavros::math::vec3(0.0f, 1.0f, 0.0f)), // right
            tavros::math::normalize(tavros::math::vec3(0.0f, 0.0f, 1.0f)), // up (перпендикуляр к forward)
            {1.0f, 2.0f, 3.0f}                                             // half extents
        );

        m_drenderer.box3d(
            obb1,
            {1.0f, 0.5f, 1.0f, 0.75f},
            tavros::renderer::debug_renderer::draw_mode::edges
        );

        m_drenderer.box3d(
            obb1,
            {0.9f, 0.7f, 1.0f, 0.2f}

        );


        m_drenderer.bezier_curve3d(
            {0.0f, 0.0f, 0.0f},       // start
            {1.0f, 0.0f, 3.0f},       // p2 (тянет вверх)
            {3.0f, 0.0f, 3.0f},       // finish (перед вершиной)
            {4.0f, 0.0f, 0.0f},       // p4 (конец)
            32,                       // segments
            {1.0f, 0.0f, 0.0f, 1.0f}, // красный
            {1.0f, 1.0f, 0.0f, 1.0f}, // жёлтый
            true,
            32.0f
        );

        // 2. Волнообразная траектория
        m_drenderer.bezier_curve3d(
            {-2.0f, 0.0f, 0.0f},      // start
            {-1.0f, 2.0f, 2.0f},      // p2
            {2.0f, -2.0f, 2.0f},      // finish
            {3.0f, 0.0f, 0.0f},       // p4
            48,
            {0.0f, 0.8f, 1.0f, 1.0f}, // голубой
            {0.8f, 0.0f, 1.0f, 1.0f}, // фиолетовый
            true,
            32.0f
        );

        // 3. Спиралевидная дуга (наклон по XZ)
        m_drenderer.bezier_curve3d(
            {0.0f, 0.0f, 0.0f},       // start
            {2.0f, 1.0f, 2.0f},       // p2
            {3.0f, -1.0f, 4.0f},      // finish
            {5.0f, 0.0f, 5.0f},       // p4
            40,
            {0.3f, 1.0f, 0.3f, 1.0f}, // зелёный
            {1.0f, 0.5f, 0.0f, 1.0f}, // оранжевый
            true,
            32.0f
        );

        // 4. Диагональная линия в пространстве (почти прямая)
        m_drenderer.bezier_curve3d(
            {-1.0f, -1.0f, 0.0f},
            {0.0f, 0.0f, 1.0f},
            {1.0f, 1.0f, 2.0f},
            {2.0f, 2.0f, 3.0f},
            22,
            {1.0f, 0.0f, 1.0f, 1.0f}, // пурпурный
            {0.0f, 1.0f, 1.0f, 1.0f}, // бирюзовый
            true,
            32.0f
        );


        m_drenderer.bezier_curve3d(
            {-10.0f, -10.0f, 2.0f},
            {-13.0f, -7.0f, 5.0f},
            {-3.0f, -7.0f, 5.0f},
            {2.0f, 2.0f, 3.0f},
            16,
            {1.0f, 0.0f, 1.0f, 1.0f}, // пурпурный
            {0.0f, 1.0f, 1.0f, 1.0f}, // бирюзовый
            true,
            32.0f
        );

        /*tavros::geometry::aabb2 text_rect = {100.0f, 100.0f, 900.0f, 500.0f};
        m_drenderer.box2d(text_rect, {1.0f, 1.0f, 1.0f, 0.2f});

        m_drenderer.draw_text2d(lorem_ipsum, 14.0f, tavros::renderer::debug_renderer::text_align::center_justify, text_rect, {0.6f, 0.8f, 1.0f, 0.8f});*/

        m_drenderer.sphere3d({{-10.0f, 10.0f, 20.0f}, 10.0f}, {1.0f, 1.0f, 0.3f, 0.3f});
        m_drenderer.sphere3d({{10.0f, -10.0f, 20.0f}, 3.0f}, {0.0f, 1.0f, 1.0f, 0.5f});
        m_drenderer.sphere3d({{10.0f, 0.0f, 20.0f}, 2.0f}, {1.0f, 0.0f, 1.0f, 0.5f});
        m_drenderer.sphere3d({{3.0f, 3.0f, -5.0f}, 1.0f}, {1.0f, 0.0f, 1.0f, 0.5f});
        m_drenderer.sphere3d({{10.0f, 5.0f, 0.0f}, 2.0f}, {1.0f, 1.0f, 1.0f, 0.75f});


        auto is_f10_released = m_input_manager.is_key_up(tavros::input::keyboard_key::k_F10);

        // update
        auto uniform_buffer_data_map = m_graphics_device->map_buffer(m_stage_buffer, 0, sizeof(m_renderer_frame_data));
        uniform_buffer_data_map.copy_from(&m_renderer_frame_data, sizeof(m_renderer_frame_data));
        m_graphics_device->unmap_buffer(m_stage_buffer);

        if (m_input_manager.is_key_up(tavros::input::keyboard_key::k_right)) {
            m_selected_font++;
        }
        if (m_input_manager.is_key_up(tavros::input::keyboard_key::k_left)) {
            m_selected_font--;
        }
        m_text_pos_y -= m_input_manager.key_hold_factor(tavros::input::keyboard_key::k_up) * 2.0f;
        m_text_pos_y += m_input_manager.key_hold_factor(tavros::input::keyboard_key::k_down) * 2.0f;


        if (m_input_manager.is_key_up(tavros::input::keyboard_key::k_7)) {
            m_rich_line_align = tavros::text::text_align::left;
        }
        if (m_input_manager.is_key_up(tavros::input::keyboard_key::k_8)) {
            m_rich_line_align = tavros::text::text_align::center;
        }
        if (m_input_manager.is_key_up(tavros::input::keyboard_key::k_9)) {
            m_rich_line_align = tavros::text::text_align::right;
        }
        if (m_input_manager.is_key_up(tavros::input::keyboard_key::k_0)) {
            m_rich_line_align = tavros::text::text_align::justify;
        }

        auto fnt = m_font_lib.fonts()[m_selected_font % m_font_lib.fonts().size()].font;
        m_rich_line.set_style(0, m_rich_line.glyphs().size(), fnt.get(), m_font_height);

        float text_rect_width = 1200.0f;

        auto layout_contaainer_size = tavros::text::text_layout::layout(m_rich_line.glyphs(), {text_rect_width, 1.5f, m_rich_line_align});

        auto text_rect_pos = tavros::math::vec2(150.0f, m_text_pos_y);
        auto text_rect_pos_max = text_rect_pos + tavros::math::vec2(text_rect_width, 700.0f);
        auto text_rect = tavros::geometry::aabb2(text_rect_pos, text_rect_pos_max);

        layout_contaainer_size.min += text_rect_pos;
        layout_contaainer_size.max += text_rect_pos;
        m_drenderer.box2d(layout_contaainer_size, {0.0f, 1.0f, 0.0f, 1.0f}, tavros::renderer::debug_renderer::draw_mode::edges);

        /*auto test_glyph_mouse_pos = m_input_manager.get_mouse_pos() - text_rect_pos;
        for (auto& g : m_rich_line.glyphs()) {
            if (g.base.bounds.contains_point(test_glyph_mouse_pos)) {
                g.params.fill_color.set(255, 0, 0, 255);
            }

            auto dbox = g.base.bounds;
            dbox.min += text_rect_pos;
            dbox.max += text_rect_pos;
            m_drenderer.box2d(dbox, {1.0f, 1.0f, 1.0f, 1.0f}, tavros::renderer::debug_renderer::draw_mode::edges);

            auto gbox = g.base.layout;
            gbox.min += text_rect_pos;
            gbox.max += text_rect_pos;
            m_drenderer.box2d(gbox, {0.0f, 1.0f, 0.0f, 1.0f}, tavros::renderer::debug_renderer::draw_mode::edges);
        }*/

        auto text_lay_rect = tavros::geometry::aabb2(150.0f, 300.0f, 150.0f + text_rect_width, 700.0f);
        m_drenderer.box2d(text_lay_rect, {1.0f, 1.0f, 1.0f, 1.0f}, tavros::renderer::debug_renderer::draw_mode::edges);

        tavros::core::string str_info;
        str_info.reserve(1024);
        str_info.append("Frame number: ");
        str_info.append(std::to_string(m_frame_number));
        str_info.append("\nAverage FPS: ");
        str_info.append(std::to_string(m_fps_meter.average_fps()));
        str_info.append("\nMedian FPS: ");
        str_info.append(std::to_string(m_fps_meter.median_fps()));
        str_info.append("\nLast frame time: ");
        str_info.append(std::to_string(m_frame_time));

        m_fps_line.set_text(str_info, m_font_lib.fonts()[0].font.get(), 48.0f);
        for (auto& it : m_fps_line.glyphs()) {
            it.params.fill_color.set(255, 200, 255, 255);
            it.params.outline_color.set(255, 255, 255, 0);
        }
        tavros::text::text_layout::layout(m_fps_line.glyphs(), {10000.0f, 1.5f, tavros::text::text_align::left});

        auto glyph_instance_text_draw_slice = m_stream_draw->slice<glyph_instance>(m_rich_line.glyphs().size());
        auto glyph_instance_fps_line_slice = m_stream_draw->slice<glyph_instance>(m_rich_line.glyphs().size());

        auto text_draw_size = fill_glyphs_instances(m_rich_line.glyphs(), glyph_instance_text_draw_slice.data(), text_rect_pos, k_sdf_size_pix / k_font_scale_pix);
        auto fps_text_draw_size = fill_glyphs_instances(m_fps_line.glyphs(), glyph_instance_fps_line_slice.data(), {16.0f, 16.0f}, k_sdf_size_pix / k_font_scale_pix);


        auto* cbuf = m_composer->create_command_queue();
        m_composer->begin_frame();

        // Copy m_renderer_frame_data to shader
        cbuf->copy_buffer(m_stage_buffer, m_uniform_buffer, sizeof(m_renderer_frame_data));

        cbuf->begin_render_pass(m_offscreen_rt->render_pass(), m_offscreen_rt->framebuffer());

        // Draw cube
        cbuf->bind_pipeline(m_mesh_rendering_pipeline);
        rhi::bind_buffer_info vert_bufs[] = {{m_mesh_vertices_buffer, 0}, {m_mesh_vertices_buffer, 0}, {m_mesh_vertices_buffer, 0}};
        cbuf->bind_vertex_buffers(vert_bufs);
        cbuf->bind_index_buffer(m_mesh_indices_buffer, rhi::index_buffer_format::u32);
        cbuf->bind_shader_binding(m_scene_binding);
        cbuf->bind_shader_binding(m_mesh_shader_binding);
        cbuf->draw_indexed(6 * 6);

        // Draw world grid
        cbuf->bind_pipeline(m_world_grid_rendering_pipeline);
        cbuf->bind_shader_binding(m_scene_binding);
        cbuf->draw(4);


        cbuf->bind_pipeline(m_sdf_font_pipeline);
        cbuf->bind_shader_binding(m_font_shader_binding);
        cbuf->bind_shader_binding(m_scene_binding);
        rhi::bind_buffer_info font_vert_bufs[] = {
            {glyph_instance_text_draw_slice.gpu_buffer(), glyph_instance_text_draw_slice.offset_bytes()},
            {glyph_instance_text_draw_slice.gpu_buffer(), glyph_instance_text_draw_slice.offset_bytes()},
            {glyph_instance_text_draw_slice.gpu_buffer(), glyph_instance_text_draw_slice.offset_bytes()},
            {glyph_instance_text_draw_slice.gpu_buffer(), glyph_instance_text_draw_slice.offset_bytes()}
        };
        cbuf->bind_vertex_buffers(font_vert_bufs);
        // cbuf->bind_index_buffer(m_mesh_indices_buffer, rhi::index_buffer_format::u32);

        rhi::scissor_info text_scissor = {
            static_cast<int32>(tavros::math::floor(text_lay_rect.left)),
            static_cast<int32>(tavros::math::floor(text_lay_rect.top)),
            static_cast<int32>(tavros::math::ceil(text_lay_rect.right - text_lay_rect.left)),
            static_cast<int32>(tavros::math::ceil(text_lay_rect.bottom - text_lay_rect.top))
        };
        cbuf->set_scissor(text_scissor);

        cbuf->draw(4, 0, static_cast<uint32>(text_draw_size), 0);


        rhi::bind_buffer_info font_vert_bufs_2[] = {
            {glyph_instance_fps_line_slice.gpu_buffer(), glyph_instance_fps_line_slice.offset_bytes()},
            {glyph_instance_fps_line_slice.gpu_buffer(), glyph_instance_fps_line_slice.offset_bytes()},
            {glyph_instance_fps_line_slice.gpu_buffer(), glyph_instance_fps_line_slice.offset_bytes()},
            {glyph_instance_fps_line_slice.gpu_buffer(), glyph_instance_fps_line_slice.offset_bytes()}
        };
        cbuf->bind_vertex_buffers(font_vert_bufs_2);

        cbuf->set_scissor({0, 0, static_cast<int32>(m_composer->width()), static_cast<int32>(m_composer->height())});
        cbuf->draw(4, 0, static_cast<uint32>(fps_text_draw_size), 0);

        m_drenderer.update();
        m_drenderer.render(cbuf);
        m_drenderer.end_frame();

        m_root_view.on_frame(cbuf, static_cast<float>(delta_time));


        cbuf->end_render_pass();

        if (is_f10_released) {
            rhi::texture_copy_region copy_rgn;
            copy_rgn.width = m_input_manager.get_window_size().width;
            copy_rgn.height = m_input_manager.get_window_size().height;
            cbuf->copy_texture_to_buffer(m_offscreen_rt->get_color_attachment(0), m_stage_upload_buffer, copy_rgn);
            copy_rgn.buffer_offset = copy_rgn.width * copy_rgn.height * 4;
            cbuf->copy_texture_to_buffer(m_offscreen_rt->get_depth_stencil_attachment(), m_stage_upload_buffer, copy_rgn);
        }

        // Draw to backbuffer
        cbuf->begin_render_pass(m_main_pass, m_composer->backbuffer());

        cbuf->bind_pipeline(m_fullscreen_quad_pipeline);
        if (m_current_buffer_output_index == 0) {
            cbuf->bind_shader_binding(m_color_shader_binding);
        } else {
            cbuf->bind_shader_binding(m_depth_stencil_shader_binding);
        }
        cbuf->draw(4);

        cbuf->end_render_pass();

        m_composer->submit_command_queue(cbuf);
        m_composer->end_frame();

        m_frame_time = static_cast<float>(frame_timer.elapsed_seconds());
        m_composer->wait_for_frame_complete();
        m_composer->present();

        if (is_f10_released) {
            auto screenshot_pixels = m_graphics_device->map_buffer(m_stage_upload_buffer);

            int width = static_cast<int>(m_input_manager.get_window_size().width);
            int height = static_cast<int>(m_input_manager.get_window_size().height);
            int channels = 4;
            int stride = width * channels;

            tavros::core::dynamic_buffer<uint8> depth_data(&m_allocator);
            depth_data.reserve(width * height);
            size_t       plane_size = static_cast<size_t>(width * height);
            const float* src = reinterpret_cast<const float*>(screenshot_pixels.data() + plane_size * channels);
            for (size_t i = 0; i < plane_size; ++i) {
                depth_data.data()[i] = static_cast<uint8>(src[i] * 255.0f);
            }

            // Save screenshot
            tavros::resources::image_codec::pixels_view im_color{static_cast<uint32>(width), static_cast<uint32>(height), channels, stride, {screenshot_pixels.data(), static_cast<size_t>(stride * height)}};
            save_image(im_color, "color.png", true);
            tavros::resources::image_codec::pixels_view im_depth{width, height, 1, width, {depth_data.data(), static_cast<size_t>(width * height)}};
            save_image(im_depth, "depth.png", true);

            m_graphics_device->unmap_buffer(m_stage_upload_buffer);
        }

        // std::this_thread::sleep_for(std::chrono::milliseconds(1));

        m_input_manager.end_frame();
    }

private:
    float m_font_height = 100.0f;

    tavros::core::mallocator                       m_allocator;
    tavros::core::unique_ptr<rhi::graphics_device> m_graphics_device;
    rhi::frame_composer*                           m_composer = nullptr;

    tavros::resources::image_codec m_imcodec;

    tavros::core::unique_ptr<tavros::renderer::render_target> m_offscreen_rt;

    rhi::pipeline_handle       m_fullscreen_quad_pipeline;
    rhi::pipeline_handle       m_mesh_rendering_pipeline;
    rhi::pipeline_handle       m_world_grid_rendering_pipeline;
    rhi::pipeline_handle       m_sdf_font_pipeline;
    rhi::render_pass_handle    m_main_pass;
    rhi::shader_binding_handle m_shader_binding;
    rhi::shader_binding_handle m_mesh_shader_binding;
    rhi::shader_binding_handle m_scene_binding;
    rhi::shader_binding_handle m_color_shader_binding;
    rhi::shader_binding_handle m_depth_stencil_shader_binding;
    rhi::shader_binding_handle m_font_shader_binding;
    rhi::texture_handle        m_texture;
    rhi::buffer_handle         m_stage_buffer;
    rhi::buffer_handle         m_stage_upload_buffer;
    rhi::buffer_handle         m_mesh_vertices_buffer;
    rhi::buffer_handle         m_mesh_indices_buffer;
    rhi::sampler_handle        m_sampler;
    rhi::buffer_handle         m_uniform_buffer;
    rhi::fence_handle          m_fence;

    tavros::renderer::debug_renderer m_drenderer;

    uint32 m_current_buffer_output_index = 0;

    tavros::core::shared_ptr<tavros::resources::resource_manager> m_resource_manager;

    tavros::input::input_manager m_input_manager;
    tavros::renderer::camera     m_camera;

    struct alignas(16) frame_data
    {
        tavros::math::mat4 view;
        tavros::math::mat4 perspective_projection;
        tavros::math::mat4 view_projection;
        tavros::math::mat4 inverse_view;
        tavros::math::mat4 inverse_projection;
        tavros::math::mat4 orto_projection;

        float frame_width = 0.0f;
        float frame_height = 0.0f;

        float near_plane = 0.0f;
        float far_plane = 0.0f;
        float view_space_depth = 0.0f;
        float aspect_ratio = 0.0f;
        float fov_y = 0.0f;
    };

    frame_data m_renderer_frame_data;

    tavros::ui::root_view m_root_view;
    tavros::ui::view      m_view1;
    tavros::ui::view      m_view2;
    tavros::ui::view      m_view3;
    tavros::ui::view      m_view4;
    tavros::ui::view      m_view5;
    tavros::ui::view      m_view6;
    tavros::ui::view      m_view7;
    tavros::ui::view      m_view8;
    tavros::ui::view      m_view9;
    tavros::ui::view      m_view10;
    tavros::ui::view      m_view11;
    tavros::ui::button    m_btn0;
    tavros::ui::button    m_btn1;

    int32 m_selected_font = 0;
    float m_text_pos_y = 300.0f;

    tavros::text::rich_line<glyph_params> m_rich_line;
    tavros::text::text_align              m_rich_line_align = tavros::text::text_align::left;
    tavros::renderer::shader_loader       m_sl;

    tavros::text::rich_line<glyph_params> m_fps_line;

    size_t    m_frame_number = 0;
    fps_meter m_fps_meter;
    float     m_frame_time = 0.0f;

    tavros::text::font_library m_font_lib;

    tavros::core::unique_ptr<tavros::renderer::gpu_stream_buffer> m_stream_draw;
};

int main()
{
    tavros::core::logger::add_consumer([](auto lvl, auto tag, auto msg) { printf("%s\n", msg.data()); });

    auto resource_manager = tavros::core::make_shared<tavros::resources::resource_manager>();
    resource_manager->mount<tavros::resources::filesystem_provider>(TAV_ASSETS_PATH, tavros::resources::resource_access::read_only);
    resource_manager->mount<tavros::resources::filesystem_provider>(TAV_ASSETS_PATH "/shaders", tavros::resources::resource_access::read_only);
    resource_manager->mount<tavros::resources::filesystem_provider>(TAV_OUTPUT_PATH, tavros::resources::resource_access::write_only);

    auto wnd = tavros::core::make_unique<main_window>("TavrosEngine", resource_manager);
    wnd->run_render_loop();

    return tavros::system::application::instance().run();
}