#include <tavros/ui/static/label.hpp>

namespace tavros::ui
{

    label_view::label_view()
        : view()
        , m_dirty(true)
    {
    }

    label_view::~label_view() noexcept
    {
    }

    math::vec2 label_view::measure(math::vec2 avail) const noexcept
    {
        auto min_sz = math::max(m_padding.size(), m_min_size);
        auto max_sz = math::max(min_sz, m_max_size);
        auto required_sz = math::clamp(m_size, min_sz, max_sz);

        auto text_layout = m_text.layout();

        return math::max(required_sz, text_layout.size() + m_padding.size());
    }

    void label_view::set_text(std::string_view str)
    {
        m_str = str;
        m_text.clear();
        m_text.append_text(str);
    }

    core::string_view label_view::text() const noexcept
    {
        return m_str;
    }

    void label_view::set_font(renderer::font_ref font) noexcept
    {
        m_text.set_font(font);
    }

    void label_view::set_font_size(float size) noexcept
    {
        m_text.set_font_size(size);
    }

    void label_view::update(float dt)
    {
    }

    void label_view::draw(const render_context& rctx)
    {
        view::draw(rctx);

        auto text_layout = m_text.layout();

        auto* r = rctx.renderer->renderer2d();

        auto cb = content_bounds();
        r->set_clip_rect(m_bounds);
        r->draw_text(m_text.text(), cb.min, 0.5f, 0.1f);
        r->reset_clip_rect();
    }

} // namespace tavros::ui
