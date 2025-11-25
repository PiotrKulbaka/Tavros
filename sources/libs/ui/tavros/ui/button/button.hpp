#pragma once

#include <tavros/ui/view.hpp>

namespace tavros::ui
{

    class button : public view
    {
    public:
        button();
        ~button() override;

        void set_text(core::string_view text);

        void update(float dt) override;
        void draw(const render_context& rctx) override;

        virtual void on_click();

        void on_mouse_down(const mouse_button_event_args&) override;
        void on_mouse_up(const mouse_button_event_args&) override;
        void on_mouse_hover() override;
        void on_mouse_leave() override;

    private:
        bool         m_is_pressed;
        bool         m_is_hovered;
        float        m_hover_time;
        core::string m_text;
    };

} // namespace tavros::ui