#pragma once

#include <tavros/core/containers/vector.hpp>
#include <tavros/core/fixed_string.hpp>
#include <tavros/tef/workspace.hpp>
#include <tavros/tef/schema.hpp>

namespace tavros::renderer
{

    class font_desc
    {
    public:
        /**
         * @brief Represents a continuous inclusive Unicode range.
         *
         * Only glyphs whose codepoints fall within the provided ranges will be loaded.
         * Ranges must be sorted in ascending order and must not overlap.
         */
        struct codepoint_range
        {
            /// First Unicode codepoint in the range (inclusive).
            char32 first_codepoint = 0;

            /// Last Unicode codepoint in the range (inclusive).
            char32 last_codepoint = 0;
        };

    public:
        font_desc() noexcept
        {
        }

        font_desc(core::string_view name, core::string_view path, core::vector<codepoint_range> rnanges) noexcept
            : m_name(name)
            , m_path(path)
            , m_codepoint_ranges(std::move(rnanges))
        {
        }

        core::string_view name() const noexcept
        {
            return m_name;
        }

        core::string_view path() const noexcept
        {
            return m_path;
        }

        core::buffer_view<codepoint_range> codepoint_ranges() const noexcept
        {
            return m_codepoint_ranges;
        }

    private:
        core::short_string            m_name;
        core::fixed_path              m_path;
        core::vector<codepoint_range> m_codepoint_ranges;
    };

} // namespace tavros::renderer

namespace tavros::tef
{
    template<>
    struct schema<tavros::renderer::font_desc>
    {
        static void serialize(node* n, const tavros::renderer::font_desc& in, core::diagnostics& ds) noexcept;
        static void deserialize(const node* n, tavros::renderer::font_desc& out, core::diagnostics& ds) noexcept;
    };
} // namespace tavros::tef
