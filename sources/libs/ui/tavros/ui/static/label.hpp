#pragma once

#include <tavros/ui/view.hpp>

namespace tavros::ui
{

    class label_view : public view
    {
    public:
        label_view();
        ~label_view() noexcept override;

        math::vec2 measure(math::vec2 avail) const noexcept override;

        // void arrange(geometry::aabb2 bounds) noexcept override;

        // math::vec2 view::measure(math::vec2 avail) const noexcept
        //{
        //     auto inner_avail = math::max(avail - m_margin.size(), math::vec2{0.0f, 0.0f});
        //     auto desired = math::min(inner_avail, math::clamp(m_size, m_min_size, m_max_size));
        //     return desired + m_margin.size();
        // }

        // void view::arrange(geometry::aabb2 bounds) noexcept
        //{
        //     auto inner = geometry::aabb2{
        //         bounds.left + m_margin.left,
        //         bounds.top + m_margin.top,
        //         bounds.right - m_margin.right,
        //         bounds.bottom - m_margin.bottom,
        //     };

        //    auto size = math::clamp(inner.size(), m_min_size, m_max_size);

        //    m_bounds = {
        //        inner.left,
        //        inner.top,
        //        inner.left + size.width,
        //        inner.top + size.height
        //    };
        //}

        void set_text(std::string_view str);

        void set_font(renderer::font_ref font) noexcept;

        void set_font_size(float size) noexcept;

        core::string_view text() const noexcept;

        void update(float dt) override;
        void draw(const render_context& rctx) override;

    private:
        core::string                m_str;
        mutable renderer::rich_text m_text;
        bool                        m_dirty;
        float                       m_font_size;
    };


} // namespace tavros::ui
