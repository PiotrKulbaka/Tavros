#pragma once

#include <tavros/ui/view.hpp>

namespace
{
    float align_offset(tavros::ui::h_align align, float avail, float size) noexcept
    {
        switch (align) {
            case tavros::ui::h_align::left:   return 0.0f;
            case tavros::ui::h_align::center: return (avail - size) * 0.5f;
            case tavros::ui::h_align::right:  return avail - size;
        }
        return 0.0f;
    }

    float align_offset(tavros::ui::v_align align, float avail, float size) noexcept
    {
        switch (align) {
            case tavros::ui::v_align::top:    return 0.0f;
            case tavros::ui::v_align::center: return (avail - size) * 0.5f;
            case tavros::ui::v_align::bottom: return avail - size;
        }
        return 0.0f;
    }
} // namespace

namespace tavros::ui
{

    class linear_view : public view
    {
    public:

        linear_view(core::string_view name) noexcept
            : view(name)
        {
        }

        void set_content_orientation(content_orient orient) noexcept
        {
            m_content_orient = orient;
        }

        content_orient content_orientation() const noexcept
        {
            return m_content_orient;
        }

        void set_spacing(float spacing) noexcept
        {
            m_spacing = spacing;
        }

        float spacing() const noexcept
        {
            return m_spacing;
        }

        math::vec2 measure(math::vec2 border_box_size) noexcept override
        {
            auto cb_size = limit_border_box_size(border_box_size) - padding().size() - border().size();
            auto total = math::vec2{};

            for (auto& child : children()) {
                if (!child.is_enabled()) {
                    continue;
                }
                auto desired = child.measure(cb_size) + child.margin().size();

                if (m_content_orient == content_orient::horizontal) {
                    total.width += desired.width;
                    total.y = std::max(total.y, desired.y);
                    if (!child.is_last()) {
                        total.width += m_spacing;
                    }
                } else if (m_content_orient == content_orient::vertical) {
                    total.width = std::max(total.width, desired.width);
                    total.y += desired.y;
                    if (!child.is_last()) {
                        total.y += m_spacing;
                    }
                }
            }

            total += padding().size() + border().size();
            return limit_border_box_size(total);
        }

        void arrange(geometry::aabb2 padding_box_bounds) noexcept override
        {
            view::arrange(padding_box_bounds);
            auto  cb = content_box();
            float cursor = (m_content_orient == content_orient::vertical) ? cb.top : cb.left;

            for (auto& child : children()) {
                if (!child.is_enabled()) {
                    continue;
                }

                math::vec2 avail = cb.size();
                if (m_content_orient == content_orient::vertical) {
                    avail.y = std::max(cb.bottom - cursor, 0.0f);
                } else {
                    avail.x = std::max(cb.right - cursor, 0.0f);
                }

                auto child_margin = child.margin();
                auto desired = child.measure(avail - child_margin.size()) + child_margin.size();

                geometry::aabb2 child_bounds;

                if (m_content_orient == content_orient::vertical) {
                    // main axis = vertical, cross axis = horizontal -> horizontal_alignment
                    float cross_avail  = cb.width();
                    float cross_size   = std::min(desired.x, cross_avail);
                    float cross_offset = align_offset(child.horizontal_alignment(), cross_avail, cross_size);

                    child_bounds = {
                        cb.left + cross_offset,
                        cursor,
                        cb.left + cross_offset + cross_size,
                        cursor + desired.y
                    };
                    cursor += desired.y + m_spacing;
                } else {
                    // main axis = horizontal, cross axis = vertical -> vertical_alignment
                    float cross_avail  = cb.height();
                    float cross_size   = std::min(desired.y, cross_avail);
                    float cross_offset = align_offset(child.vertical_alignment(), cross_avail, cross_size);

                    child_bounds = {
                        cursor,
                        cb.top + cross_offset,
                        cursor + desired.x,
                        cb.top + cross_offset + cross_size
                    };
                    cursor += desired.x + m_spacing;
                }

                child.arrange(child_bounds.shrinked(child_margin));
            }
        }

    private:
        content_orient m_content_orient = content_orient::vertical;
        float          m_spacing = 0.0f;
    };

} // namespace tavros::ui
