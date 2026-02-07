#include <tavros/core/exception.hpp>

#include <tavros/core/debug/unreachable.hpp>

namespace tavros::core
{

    string_view to_string(file_error_tag tag) noexcept
    {
        using enum file_error_tag;
        switch (tag) {
        case not_found:
            return "not_found";
        case permission_denied:
            return "permission_denied";
        case open_failed:
            return "open_failed";
        case read_error:
            return "read_error";
        case write_error:
            return "write_error";
        case invalid_path:
            return "invalid_path";
        case invalid_argument:
            return "invalid_argument";
        case is_directory:
            return "is_directory";
        case other:
            "other";
        }

        TAV_UNREACHABLE();
    }

    string_view to_string(format_error_tag tag) noexcept
    {
        using enum format_error_tag;
        switch (tag) {
        case syntax:
            return "syntax";
        case missing_field:
            return "missing_field";
        case invalid_type:
            return "invalid_type";
        case invalid_value:
            return "invalid_value";
        case unsupported:
            return "unsupported";
        }

        TAV_UNREACHABLE();
    }

} // namespace tavros::core
