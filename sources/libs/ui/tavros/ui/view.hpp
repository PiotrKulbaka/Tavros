#pragma once

#include <tavros/ui/base.hpp>
#include <tavros/ui/ui_context.hpp>
#include <tavros/core/math.hpp>
#include <tavros/core/containers/hierarchy.hpp>

namespace tavros::ui
{

    class root_view;

    class view : public core::hierarchy<view>
    {
    public:
        view(core::string_view name) noexcept;
        virtual ~view() noexcept;

        void set_visible(bool v) noexcept;
        bool is_visible() const noexcept;

        void set_enabled(bool e) noexcept;
        bool is_enabled() const noexcept;

        void       set_size(math::vec2 size) noexcept;         // bounds size
        math::vec2 size() const noexcept;                      // bounds size

        void       set_min_size(math::vec2 min_size) noexcept; // min bounds size
        math::vec2 min_size() const noexcept;                  // min bounds size

        void       set_max_size(math::vec2 max_size) noexcept; // max bounds size
        math::vec2 max_size() const noexcept;                  // max bounds size

        void       set_margin(const thickness4& margin) noexcept;
        thickness4 margin() const noexcept;

        void       set_border(const thickness4& border) noexcept;
        thickness4 border() const noexcept;

        void       set_padding(const thickness4& padding) noexcept;
        thickness4 padding() const noexcept;

        void    set_border_radius(const radius4& radius) noexcept;
        radius4 border_radius() const noexcept;

        aabb2 margin_box() const noexcept;
        aabb2 border_box() const noexcept;
        aabb2 padding_box() const noexcept;
        aabb2 content_box() const noexcept;

        void    set_horizontal_alignment(h_align align) noexcept; // Self alignment
        h_align horizontal_alignment() const noexcept;            // Self alignment

        void    set_vertical_alignment(v_align align) noexcept;   // Self alignment
        v_align vertical_alignment() const noexcept;              // Self alignment

        //
        virtual void resolve(ui_context& ctx);

        virtual math::vec2 measure(math::vec2 available_border_box_size) noexcept;
        virtual void       arrange(aabb2 border_box_bounds) noexcept;

        // Update & draw
        virtual void update(float dt);
        virtual void draw(ui_context& ctx);

        virtual bool hit_test(math::vec2 pos) const noexcept;

        // Events
        virtual void on_mouse_down(const mouse_button_event_args&);
        virtual void on_mouse_move(const mouse_move_event_args&);
        virtual void on_mouse_up(const mouse_button_event_args&);
        virtual void on_mouse_wheel(const mouse_wheel_event_args&);
        virtual void on_mouse_enter();
        virtual void on_mouse_leave();
        virtual void on_key_down(const keyboard_key_event_args&);
        virtual void on_key_up(const keyboard_key_event_args&);
        virtual void on_key_press(const key_press_event_args&);

    protected:
        math::vec2 limit_border_box_size(math::vec2 border_box_size) const noexcept;
        math::vec2 resolve_content_avail(math::vec2 available_border_box_size) const noexcept;
        math::vec2 resolve_desired_size(math::vec2 content_size) const noexcept;

    protected:
        view_name  m_name;
        bool       m_is_entered;
        bool       m_is_visible;
        bool       m_is_enabled;
        h_align    m_h_align;
        v_align    m_v_align;
        math::vec2 m_border_box_desired_size;
        math::vec2 m_border_box_min_size;
        math::vec2 m_border_box_max_size;
        thickness4 m_padding;
        thickness4 m_border;
        thickness4 m_margin;
        radius4    m_border_radius;
        aabb2      m_border_box;
    };

} // namespace tavros::ui
