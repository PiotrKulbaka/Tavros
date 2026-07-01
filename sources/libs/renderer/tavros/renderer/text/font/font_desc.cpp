#include <tavros/renderer/text/font/font_desc.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/core/logger/diagnostics.hpp>
#include <tavros/core/debug/unreachable.hpp>

#include <tavros/tef/helpers.hpp>

namespace
{
    tavros::core::logger logger("font_desc");
    using diagnostics = tavros::core::diagnostics;
    using font_desc = tavros::renderer::font_desc;
    using codepoint_range = tavros::renderer::font_desc::codepoint_range;

    std::optional<font_desc> parse_font_desc(const tavros::tef::node* n, tavros::core::string_view name, diagnostics& ds) noexcept
    {
        TAV_ASSERT(n);

        bool valid = true;

        tavros::core::vector<codepoint_range> ranges;
        tavros::core::string_view             path = "";

        if (const auto* p = n->resolve_path("path")) {
            if (!p->is_string()) {
                ds.error("'path' at '{}' must be a string path.", p->path());
                valid = false;
            } else {
                path = p->value_or<tavros::core::string_view>("");
                if (path.empty()) {
                    ds.error("'path' at '{}' must not be empty.", p->path());
                    valid = false;
                }
            }
        } else {
            ds.error("Missing required field 'path' at '{}'.", n->path());
            valid = false;
        }

        constexpr auto is_char32 = [](const tavros::tef::node* node) noexcept {
            return node->is_integer();
        };

        if (const auto* cp = n->resolve_path("codepoints")) {
            while (cp) {
                uint32 first_cp = 0;
                uint32 last_cp = 0;
                if (cp->is_container()) {
                    valid &= read_required_clamped<uint32>(cp, "first", 0, 0xffffffff, first_cp, is_char32, "Unicode codepoint", ds);
                    valid &= read_required_clamped<uint32>(cp, "last", 0, 0xffffffff, last_cp, is_char32, "Unicode codepoint", ds);
                } else {
                    ds.error("'codepoints' at '{}' must be an object.", cp->path());
                    valid = false;
                }

                if (valid) {
                    if (first_cp > last_cp) {
                        ds.warning(
                            "'codepoints' range at '{}' is invalid: first codepoint (U+{:04X}) is greater than last codepoint (U+{:04X}).",
                            cp->path(),
                            first_cp,
                            last_cp
                        );
                        auto temp = first_cp;
                        first_cp = last_cp;
                        last_cp = temp;
                    }

                    ranges.push_back({static_cast<char32>(first_cp), static_cast<char32>(last_cp)});
                }

                cp = cp->next();
                if (cp && cp->has_key()) {
                    break;
                }
            }
        }

        if (valid) {
            return font_desc(n->path(), path, std::move(ranges));
        }
        return std::nullopt;
    }
} // namespace

namespace tavros::tef
{
    void schema<tavros::renderer::font_desc>::serialize(node* n, const tavros::renderer::font_desc& in, core::diagnostics& ds) noexcept
    {
        TAV_ASSERT(false);
        // TODO: Implement serialization
    }

    void schema<tavros::renderer::font_desc>::deserialize(const node* n, tavros::renderer::font_desc& out, core::diagnostics& ds) noexcept
    {
        // TAV_ASSERT(n);
        if (!n) {
            ds.error("Null node provided for deserializing font config.");
            return;
        }

        if (auto result = parse_font_desc(n, n->path(), ds)) {
            out = *result;
        } else {
            ds.error("Failed to parse material configuration at '{}'.", n->path());
        }
    }

} // namespace tavros::tef