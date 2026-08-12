#include <tavros/renderer/shader_bindings/shape_collection.hpp>

namespace
{
    enum class shape_t : uint8
    {
        line = 0,
        circle = 1,
        round_rect = 2,
    };
}

namespace tavros::renderer
{

    shape_id shape_collection::add_line(const math::vec2& from, const math::vec2& to, float thickness, float dash, float gap)
    {
        const auto id = static_cast<uint32>(m_shapes.size()); // + 1 because shape ID 0 - is reserved
        m_shapes.push_back(shape_type{static_cast<uint32>(shape_t::line), 0.0f, 0, 0, {from, to}, {thickness, dash, gap, 0.0f}});
        return {id};
    }

    shape_id shape_collection::add_circle(const math::vec2& center, float radius)
    {
        const auto id = static_cast<uint32>(m_shapes.size()); // + 1 because shape ID 0 - is reserved
        m_shapes.push_back(shape_type{static_cast<uint32>(shape_t::circle), 0.0f, 0, 0, {center, radius, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f}});
        return {id};
    }
        
    shape_id shape_collection::add_round_rect(const math::vec2& center, const math::vec2& half_sizes, const geometry::radius4& radius, float rot)
    {
        const auto id = static_cast<uint32>(m_shapes.size()); // + 1 because shape ID 0 - is reserved
        m_shapes.push_back(shape_type{
            static_cast<uint32>(shape_t::round_rect),
            rot,
            0, 0,
            {center, half_sizes}, 
            {radius.top_left, radius.top_right, radius.bottom_right,radius.bottom_left}
        });
        return {id};
    }

    size_t shape_collection::upload_size_bytes() const noexcept
    {
        return sizeof(shape_buffer) + (m_shapes.size() + 1) * sizeof(shape_type); // size() + 1 because shape ID 0 - is reserved
    }

    rhi::buffer_binding shape_collection::upload(gpu_stream_buffer& sb) const noexcept
    {
        auto slice = sb.slice<uint8>(upload_size_bytes());

        shape_buffer b;
        b.size = static_cast<uint32>(m_shapes.size());

        shape_type s_zero; // Shape with ID 0 will never be used

        auto off = slice.data().copy_from(&b, sizeof(shape_buffer), 0);
        off = slice.data().copy_from(&s_zero, sizeof(shape_type), off);
        slice.data().copy_from(m_shapes.data(), m_shapes.size() * sizeof(shape_type), off);
        return {slice.gpu_buffer(), static_cast<uint32>(slice.offset_bytes()), static_cast<uint32>(slice.size_bytes()), k_shape_collection_binding_index};
    }

    void shape_collection::clear() noexcept
    {
        m_shapes.clear();
    }

}
