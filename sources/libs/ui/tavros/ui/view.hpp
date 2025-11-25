#pragma once

#include <tavros/ui/base.hpp>
#include <tavros/core/containers/hierarchy.hpp>

namespace tavros::ui
{

    class root_view;


    class view : public core::hierarchy<view>
    {
    public:
        view();
        virtual ~view() = default;

        // Position & size
        rect2 padding_rect() const;
        rect2 content_rect() const;

        void     set_padding(padding2 pad);
        padding2 padding() const;

        void   set_position(const point2 pos);
        point2 position() const;
        point2 absolute_position() const;
        point2 absolute_content_position() const;

        void  set_size(const size2 size);
        size2 size() const;
        size2 outer_size() const;

        void set_enabled(bool enabled);
        bool is_enabled() const;


        // Update & draw
        virtual void update(float dt);
        virtual void draw(const render_context& rctx);

        // Events
        virtual void on_mouse_down(const mouse_button_event_args&);
        virtual void on_mouse_move(const mouse_move_event_args&);
        virtual void on_mouse_up(const mouse_button_event_args&);
        virtual void on_mouse_wheel(const mouse_wheel_event_args&);
        virtual void on_mouse_hover();
        virtual void on_mouse_leave();
        virtual void on_key_down(const keyboard_key_event_args&);
        virtual void on_key_up(const keyboard_key_event_args&);
        virtual void on_key_press(const key_press_event_args&);

        virtual void on_parent_resized(const size2 parent_size);

        // Hit
        virtual bool test_hit(point2 p) const noexcept;

    protected:
        bool is_subtree_enabled() const;

    protected:
        padding2 m_pad;
        point2   m_pos;
        size2    m_size;


        float m_hover_factor;
        float m_click_factor;
        bool  m_is_hovered;
        bool  m_is_clicked;

        bool m_is_enabled;
    };

} // namespace tavros::ui
