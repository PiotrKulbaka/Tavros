#pragma once

#include <tavros/ui/base.hpp>
#include <tavros/core/containers/hierarchy.hpp>
#include <tavros/core/math.hpp>

namespace tavros::ui
{

    class root_view;

    class view : public core::hierarchy<view>
    {
    public:
        view() noexcept;
        virtual ~view() noexcept;

        void set_visible(bool v) noexcept;
        void set_enabled(bool e) noexcept;
        bool is_visible() const noexcept;
        bool is_enabled() const noexcept;

        void       set_size(math::vec2 size) noexcept;         // bounds size
        void       set_min_size(math::vec2 min_size) noexcept; // min bounds size
        void       set_max_size(math::vec2 max_size) noexcept; // max bounds size
        math::vec2 size() const noexcept;                      // bounds size
        math::vec2 min_size() const noexcept;                  // min bounds size
        math::vec2 max_size() const noexcept;                  // max bounds size

        void                 set_padding(geometry::thickness4 pad) noexcept;
        void                 set_margin(geometry::thickness4 margin) noexcept;
        geometry::thickness4 padding() const noexcept;
        geometry::thickness4 margin() const noexcept;

        // Returns content size + padding
        virtual math::vec2 measure(math::vec2 avail) const noexcept;
        // bounds - padding box
        virtual void arrange(geometry::aabb2 bounds) noexcept;

        geometry::aabb2 margin_bounds() const noexcept;
        geometry::aabb2 bounds() const noexcept;
        geometry::aabb2 content_bounds() const noexcept;

        // Update & draw
        virtual void update(float dt);
        virtual void draw(const render_context& rctx);

        virtual bool hit_test(math::vec2 pos) const noexcept;

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

    protected:
        math::vec2 limit_content_size(math::vec2 size) const noexcept;

    protected:
        bool                 m_is_visible;
        bool                 m_is_enabled;
        math::vec2           m_size;
        math::vec2           m_min_size;
        math::vec2           m_max_size;
        geometry::thickness4 m_padding;
        geometry::thickness4 m_margin;
        geometry::aabb2      m_bounds;
        bool                 m_is_hover = false;
    };

} // namespace tavros::ui
