#pragma once

#include <tavros/ui/view.hpp>

namespace tavros::ui
{

    class linear_view : public view
    {
    public:
        enum class content_orientation
        {
            horizontal,
            vertical
        };

        linear_view() noexcept = default;

        void set_orientation(content_orientation o) noexcept
        {
            m_orientation = o;
        }

        void set_spacing(float spacing) noexcept
        {
            m_spacing = spacing;
        }

        content_orientation orientation() const noexcept
        {
            return m_orientation;
        }
        float get_spacing() const noexcept
        {
            return m_spacing;
        }

        math::vec2 measure(math::vec2 avail) const noexcept override
        {
            auto       cb_size = limit_content_size(avail - m_padding.size());
            math::vec2 total;

            for (auto& child : children()) {
                auto desired = child.measure(cb_size) + child.margin().size();

                if (m_orientation == content_orientation::vertical) {
                    total.width = std::max(total.width, desired.width);
                    total.y += desired.y;
                    if (!child.is_last()) {
                        total.y += m_spacing;
                    }
                } else {
                    total.width += desired.width;
                    total.y = std::max(total.y, desired.y);
                    if (!child.is_last()) {
                        total.width += m_spacing;
                    }
                }
            }

            // Добавляем padding обратно
            total += m_padding.size();

            return {
                std::clamp(total.x, m_min_size.x, m_max_size.x),
                std::clamp(total.y, m_min_size.y, m_max_size.y),
            };
        }

        void arrange(geometry::aabb2 bounds) noexcept override
        {
            view::arrange(bounds);       // сохраняет m_bounds

            auto  cb = content_bounds(); // m_bounds минус padding
            float cursor = (m_orientation == content_orientation::vertical) ? cb.top : cb.left;

            for (auto& child : children()) {
                math::vec2 avail = {cb.width(), cb.height()};

                // Ограничиваем доступное место оставшимся по оси
                if (m_orientation == content_orientation::vertical) {
                    avail.y = std::max(cb.bottom - cursor, 0.0f);
                } else {
                    avail.x = std::max(cb.right - cursor, 0.0f);
                }

                auto child_margin = child.margin();
                auto desired = child.measure(avail) + child_margin.size();

                geometry::aabb2 child_bounds;
                if (m_orientation == content_orientation::vertical) {
                    child_bounds = {cb.left, cursor, cb.right, cursor + desired.y};
                    cursor += desired.y + m_spacing;
                } else {
                    child_bounds = {cursor, cb.top, cursor + desired.x, cb.bottom};
                    cursor += desired.x + m_spacing;
                }

                child.arrange({child_bounds.left + child_margin.left, child_bounds.top + child_margin.top, child_bounds.right - child_margin.right, child_bounds.bottom - child_margin.bottom});
            }
        }

    private:
        content_orientation m_orientation = content_orientation::vertical;
        float               m_spacing = 0.0f;
    };

} // namespace tavros::ui
