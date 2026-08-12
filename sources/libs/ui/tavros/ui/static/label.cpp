#include <tavros/ui/static/label.hpp>

namespace tavros::ui
{

    label_view::label_view(core::string_view name)
        : view(name)
        , m_font_family("")
        , m_dirty_font(true)
        , m_dirty_text(true)
        , m_is_word_wrap_enabled(false)
        , m_str(name)
        , m_text_layout(0.0f, 0.0f, 0.0f, 0.0f)
        , m_text()
    {
    }

    label_view::~label_view() noexcept
    {
    }

    void label_view::set_text(std::string_view str)
    {
        m_str = str;
        m_dirty_text = true;
    }

    void label_view::set_font(std::string_view font_family) noexcept
    {
        m_font_family = font_family;
        m_dirty_font = true;
    }

    void label_view::set_font_size(float size) noexcept
    {
        m_text.set_font_size(size);
    }

    void label_view::set_text_color(math::color32 color) noexcept
    {
        m_text.set_fill_color(color);
    }

    void label_view::set_outline_color(math::color32 color) noexcept
    {
        m_text.set_outline_color(color);
    }

    void label_view::set_word_wrap_enabled(bool enabled) noexcept
    {
        m_is_word_wrap_enabled = enabled;
    }

    bool label_view::is_word_wrap_enabled() const noexcept
    {
        return m_is_word_wrap_enabled;
    }

    void label_view::resolve(ui_context& ctx)
    {
        if (m_dirty_font) {
            auto fnt = ctx.renderer().resource_manager()->load_font(m_font_family);
            if (fnt.is_failed()) {
                ctx.renderer().resource_manager()->release_font(fnt);
                fnt = ctx.renderer().resource_manager()->default_font();
            }
            m_text.set_font(fnt);
            m_dirty_font = false;
        }

        if (m_dirty_text) {
            m_text.clear();
            m_text.append_text(m_str);
            m_dirty_text = false;
        }
    }

    math::vec2 label_view::measure(math::vec2 available_border_box_size) noexcept
    {
        // content box size (auto/fixed by axes, border + padding already subtracted)
        const auto content_avail = resolve_content_avail(available_border_box_size);

        m_text.set_line_wrap_width(m_is_word_wrap_enabled ? content_avail.width : -1.0f);
        m_text_layout = m_text.layout();

        // Converting measured content to the final border-box of the desired size + clamp
        return resolve_desired_size(m_text_layout.size());
    }

    core::string_view label_view::text() const noexcept
    {
        return m_str;
    }

    void label_view::update(float dt)
    {
    }

    void label_view::draw(ui_context& ctx)
    {
        view::draw(ctx);

        auto text_layout = m_text.layout();

        auto* r = ctx.renderer().renderer2d();

        auto cb = content_box();
       /* r->set_clip_rect(padding_box());
        r->draw_text(m_text.text(), cb.min, 0.5f, 0.1f);
        r->reset_clip_rect();*/
    }

} // namespace tavros::ui
