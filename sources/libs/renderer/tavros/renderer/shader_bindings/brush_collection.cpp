#include <tavros/renderer/shader_bindings/brush_collection.hpp>

namespace
{
    enum class fill_t : uint8
    {
        linear_gradient = 0,
        radial_gradient = 1,
    };
}

namespace tavros::renderer
{

    brush_id brush_collection::add_solid(math::color32 color)
    {
        const auto id = static_cast<uint32>(m_brushes.size()); // + 1 because burush ID 0 - is reserved
        m_brushes.push_back(brush_type{0, 0, 1, color, {}, {}});
        return {id};
    }

    brush_id brush_collection::add_linear_gradient(paint_view paint, math::vec2 from, math::vec2 to)
    {
        const auto id = static_cast<uint32>(m_brushes.size()) + 1; // + 1 because burush ID 0 - is reserved
        m_brushes.push_back(brush_type{static_cast<uint32>(fill_t::linear_gradient), paint.first, paint.number, {}, from, to});
        return {id};
    }

    brush_id brush_collection::add_radial_gradient(paint_view paint, math::vec2 center, float radius)
    {
        const auto id = static_cast<uint32>(m_brushes.size()) + 1; // + 1 because burush ID 0 - is reserved
        m_brushes.push_back(brush_type{static_cast<uint32>(fill_t::radial_gradient), paint.first, paint.number, {}, center, {radius, 0.0f}});
        return {id};
    }

    size_t brush_collection::upload_size_bytes() const noexcept
    {
        return sizeof(brush_buffer) + (m_brushes.size() + 1) * sizeof(brush_type); // size() + 1 because burush ID 0 - is reserved
    }

    rhi::buffer_binding brush_collection::upload(gpu_stream_buffer& sb) const noexcept
    {
        auto slice = sb.slice<uint8>(upload_size_bytes());

        brush_buffer b;
        b.size = static_cast<uint32>(m_brushes.size());
        b.no_brush_color = math::color32(0xFFFFFFFF);

        brush_type b_zero; // Brush with ID 0 will never be used

        auto off = slice.data().copy_from(&b, sizeof(brush_buffer), 0);
        off = slice.data().copy_from(&b_zero, sizeof(brush_type), off);
        slice.data().copy_from(m_brushes.data(), m_brushes.size() * sizeof(brush_type), off);
        return {slice.gpu_buffer(), static_cast<uint32>(slice.offset_bytes()), static_cast<uint32>(slice.size_bytes()), k_brush_collection_binding_index};
    }

    void brush_collection::clear() noexcept
    {
        m_brushes.clear();
    }

} // namespace tavros::renderer
