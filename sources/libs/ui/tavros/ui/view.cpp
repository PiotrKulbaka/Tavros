#include <tavros/ui/view.hpp>

#include <tavros/core/debug/unreachable.hpp>

namespace tavros::ui
{

    view::view(core::string_view name) noexcept
        : m_name(name)
        , m_is_entered(false)
        , m_is_visible(true)
        , m_is_enabled(true)
        , m_h_align(h_align::left)
        , m_v_align(v_align::top)
        , m_border_box_desired_size(0.0f, 0.0f)
        , m_border_box_min_size(0.0f, 0.0f)
        , m_border_box_max_size(k_max_ui_size, k_max_ui_size)
        , m_padding(0.0f, 0.0f, 0.0f, 0.0f)
        , m_border(0.0f, 0.0f, 0.0f, 0.0f)
        , m_margin(0.0f, 0.0f, 0.0f, 0.0f)
        , m_border_radius(0.0f, 0.0f, 0.0f, 0.0f)
        , m_border_box(0.0f, 0.0f, 0.0f, 0.0f)
    {
    }

    view::~view() noexcept
    {
    }

    void view::set_visible(bool v) noexcept
    {
        m_is_visible = v;
    }

    void view::set_enabled(bool e) noexcept
    {
        m_is_enabled = e;
    }

    bool view::is_visible() const noexcept
    {
        return m_is_visible;
    }

    bool view::is_enabled() const noexcept
    {
        return m_is_enabled;
    }

    void view::set_size(math::vec2 size) noexcept
    {
        m_border_box_desired_size = size;
    }

    math::vec2 view::size() const noexcept
    {
        return m_border_box_desired_size;
    }

    void view::set_min_size(math::vec2 min_size) noexcept
    {
        m_border_box_min_size = min_size;
    }

    math::vec2 view::min_size() const noexcept
    {
        return m_border_box_min_size;
    }

    void view::set_max_size(math::vec2 max_size) noexcept
    {
        m_border_box_max_size = max_size;
    }

    math::vec2 view::max_size() const noexcept
    {
        return m_border_box_max_size;
    }

    void view::set_margin(const thickness4& margin) noexcept
    {
        m_margin = margin;
    }

    thickness4 view::margin() const noexcept
    {
        return m_margin;
    }

    void view::set_border(const thickness4& border) noexcept
    {
        m_border = border;
    }

    thickness4 view::border() const noexcept
    {
        return m_border;
    }

    void view::set_padding(const thickness4& padding) noexcept
    {
        m_padding = padding;
    }

    thickness4 view::padding() const noexcept
    {
        return m_padding;
    }

    void view::set_border_radius(const radius4& radius) noexcept
    {
        m_border_radius = radius;
    }

    radius4 view::border_radius() const noexcept
    {
        return m_border_radius;
    }

    aabb2 view::margin_box() const noexcept
    {
        return m_border_box.expanded(m_margin);
    }

    aabb2 view::border_box() const noexcept
    {
        return m_border_box;
    }

    aabb2 view::padding_box() const noexcept
    {
        return m_border_box.shrinked(m_border);
    }

    aabb2 view::content_box() const noexcept
    {
        return padding_box().shrinked(m_padding);
    }

    void view::set_horizontal_alignment(h_align align) noexcept
    {
        m_h_align = align;
    }

    h_align view::horizontal_alignment() const noexcept
    {
        return m_h_align;
    }

    void view::set_vertical_alignment(v_align align) noexcept
    {
        m_v_align = align;
    }

    v_align view::vertical_alignment() const noexcept
    {
        return m_v_align;
    }

    void view::resolve(ui_context& ctx)
    {
    }

    math::vec2 view::measure(math::vec2 available_border_box_size) noexcept
    {
        return limit_border_box_size(m_border_box_desired_size);
    }

    void view::arrange(aabb2 border_box_bounds) noexcept
    {
        const auto sz = limit_border_box_size(border_box_bounds.size());
        m_border_box = {border_box_bounds.min, border_box_bounds.min + sz};
    }

    void view::update(float dt)
    {
        if (!is_visible()) {
            return;
        }
        for (auto& child : children()) {
            child.update(dt);
        }
    }

    void view::draw(ui_context& ctx)
    {
        if (!is_visible()) {
            return;
        }

        for (auto& child : children()) {
            child.draw(ctx);
        }

        const auto& dbg = ctx.debug_info();
        if (dbg.debug_draw) {
            auto& r = ctx.ui_renderer();

            if (m_is_entered) {
                auto cb = content_box();
                auto pb = padding_box();
                auto bb = border_box();
                auto mb = margin_box();

                if (!cb.is_empty()) {
                    r.quad(cb).fill_color(dbg.content_box_color).draw();
                }

                if (!m_padding.is_zero()) {
                    r.quad(pb).cut_quad(cb).fill_color(dbg.padding_box_color).draw();
                }

                if (!m_border.is_zero()) {
                    r.quad(bb).cut_quad(pb).fill_color(dbg.border_box_color).draw();
                }

                if (!m_margin.is_zero()) {
                    r.quad(mb).cut_rect(bb).fill_color(dbg.margin_box_color).draw();
                }
            }

            auto ob = m_border_box.expanded(dbg.hit_box_line_thickness * 0.5);
            auto ib = m_border_box.shrinked(dbg.hit_box_line_thickness * 0.5);
            r.quad(ob).cut_quad(ib).fill_color(dbg.hit_box_color).draw();
        }
    }

    bool view::hit_test(math::vec2 pos) const noexcept
    {
        return m_border_box.contains_point(pos);
    }

    void view::on_mouse_down(const mouse_button_event_args&)
    {
    }

    void view::on_mouse_move(const mouse_move_event_args&)
    {
    }

    void view::on_mouse_up(const mouse_button_event_args&)
    {
    }

    void view::on_mouse_wheel(const mouse_wheel_event_args&)
    {
    }

    void view::on_mouse_enter()
    {
        m_is_entered = true;
    }

    void view::on_mouse_leave()
    {
        m_is_entered = false;
    }

    void view::on_key_down(const keyboard_key_event_args&)
    {
    }

    void view::on_key_up(const keyboard_key_event_args&)
    {
    }

    void view::on_key_press(const key_press_event_args&)
    {
    }

    math::vec2 view::limit_border_box_size(math::vec2 border_box_size) const noexcept
    {
        const auto min_sz = math::max(m_padding.size() + m_border.size(), m_border_box_min_size);
        const auto max_sz = math::max(min_sz, m_border_box_max_size);
        return math::clamp(border_box_size, min_sz, max_sz);
    }

    math::vec2 view::resolve_content_avail(math::vec2 available_border_box_size) const noexcept
    {
        const bool fixed_width = m_border_box_desired_size.width > 0.0f;
        const bool fixed_height = m_border_box_desired_size.height > 0.0f;

        const auto pad_size = m_padding.size() + m_border.size();
        const auto min_sz = math::max(math::vec2(0.0f, 0.0f), m_border_box_min_size - pad_size);
        const auto max_sz = math::max(min_sz, m_border_box_max_size - pad_size);
        const auto measure_border_box_size = math::vec2{
            fixed_width ? math::min(m_border_box_desired_size.width, available_border_box_size.width) : available_border_box_size.width,
            fixed_height ? math::min(m_border_box_desired_size.height, available_border_box_size.height) : available_border_box_size.height
        };

        return math::clamp(measure_border_box_size - pad_size, min_sz, max_sz);
    }

    math::vec2 view::resolve_desired_size(math::vec2 content_size) const noexcept
    {
        const bool fixed_width = m_border_box_desired_size.width > 0.0f;
        const bool fixed_height = m_border_box_desired_size.height > 0.0f;

        auto min_desired_size = content_size + padding().size() + border().size();

        math::vec2 desired;
        desired.width = fixed_width ? m_border_box_desired_size.width : min_desired_size.width;
        desired.height = fixed_height ? m_border_box_desired_size.height : min_desired_size.height;

        return limit_border_box_size(desired);
    }

} // namespace tavros::ui
