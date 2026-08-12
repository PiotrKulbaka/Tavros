#include <tavros/renderer/shader_bindings/paint_collection.hpp>

namespace tavros::renderer
{

    paint_view paint_collection::add_paint(core::buffer_view<color_stop> color_stops)
    {
        const auto first = static_cast<uint32>(m_paints.size());
        const auto number = static_cast<uint32>(color_stops.size());
        m_paints.append_range(color_stops);
        return {first, number};
    }

    size_t paint_collection::upload_size_bytes() const noexcept
    {
        return m_paints.size() * sizeof(color_stop) + sizeof(paint_buffer);
    }

    rhi::buffer_binding paint_collection::upload(gpu_stream_buffer& sb) const noexcept
    {
        auto slice = sb.slice<uint8>(upload_size_bytes());

        paint_buffer b;
        b.size = static_cast<uint32>(m_paints.size());
        b.reserved = 0;

        auto off = slice.data().copy_from(&b, sizeof(paint_buffer), 0);
        slice.data().copy_from(m_paints.data(), m_paints.size() * sizeof(color_stop), off);
        return {slice.gpu_buffer(), static_cast<uint32>(slice.offset_bytes()), static_cast<uint32>(slice.size_bytes()), k_paint_collection_binding_index};
    }

    core::buffer_view<color_stop> paint_collection::paints() const noexcept
    {
        return m_paints;
    }

    void paint_collection::clear() noexcept
    {
        m_paints.clear();
    }

} // namespace tavros::renderer
