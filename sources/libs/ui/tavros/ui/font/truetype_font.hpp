#pragma once

#include <tavros/ui/base.hpp>
#include <tavros/core/containers/unordered_map.hpp>
#include <tavros/core/memory/dynamic_buffer.hpp>
#include <tavros/core/pimpl.hpp>

namespace tavros::ui
{

    class truetype_font : core::noncopyable
    {
    public:
        struct glyph_metrics
        {
            uint32 codepoint = 0;

            float advance_x = 0.0f; // ���������� �� ���������� ������� �� X
            float advance_y = 0.0f; // ���������� �� ���������� ������� �� Y, ������ 0 ��� �������������� �������

            float bearing_x = 0.0f; // �������� ����� ������������ ������� �� X
            float bearing_y = 0.0f; // �������� ����� ������������ ������� ����� �� Y

            float width = 0.0f;     // ������ ����� � ��������
            float height = 0.0f;    // ������ ����� � ��������
        };

        struct font_metrics
        {
            float ascent_y = 0.0f;   // ������ ��� ������� ������
            float descent_y = 0.0f;  // ������� ��� ������� ������
            float line_gap_y = 0.0f; // �������� ����� ��������
        };

        using glyph_index = uint32;

    public:
        truetype_font();
        ~truetype_font();

        bool init(core::dynamic_buffer<uint8> font_data);
        void shutdown();
        bool is_init();

        font_metrics get_font_metrics();

        glyph_index get_glyph_index(uint32 codepoint);

        glyph_metrics get_glyph_metrics(glyph_index glyph);

        float get_kerning(glyph_index glyph_left, glyph_index glyph_right);

        bool make_glyph_sdf(glyph_index glyph, tavros::core::buffer_span<uint8> pixels, uint32 stride, float font_height, float sdf_pad);

    private:
        core::dynamic_buffer<uint8>                     m_data;
        font_metrics                                    m_font_metrics;
        core::unordered_map<glyph_index, glyph_metrics> m_glyph_metrics;
        bool                                            m_is_init;
        float                                           m_scale = 0.0f;
        struct impl;
        core::pimpl<impl, 160, 8> m_impl;
    };

} // namespace tavros::ui
