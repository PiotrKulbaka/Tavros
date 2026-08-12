#pragma once

#include <tavros/ui/view.hpp>

namespace tavros::ui
{

    class label_view : public view
    {
    public:
        label_view(core::string_view name);
        ~label_view() noexcept override;

        void set_text(std::string_view str);

        void set_font(std::string_view font_family) noexcept;

        void set_font_size(float size) noexcept;

        void set_text_color(math::color32 color) noexcept;

        void set_outline_color(math::color32 color) noexcept;

        void set_word_wrap_enabled(bool enabled) noexcept;

        bool is_word_wrap_enabled() const noexcept;

        void resolve(ui_context& ctx) override;

        math::vec2 measure(math::vec2 available_border_box_size) noexcept override;

        core::string_view text() const noexcept;

        void update(float dt) override;

        void draw(ui_context& ctx) override;

    private:
        core::short_string  m_font_family;
        bool                m_dirty_font;
        bool                m_dirty_text;
        bool                m_is_word_wrap_enabled;
        core::string        m_str;
        geometry::aabb2     m_text_layout;
        renderer::rich_text m_text;
    };


} // namespace tavros::ui
