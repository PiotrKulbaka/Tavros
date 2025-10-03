#pragma once

#if defined(_MSC_VER)
#  pragma warning(push, 0)
#pragma warning(disable: 4506)
#elif defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wsign-conversion"
#  pragma clang diagnostic ignored "-Wold-style-cast"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wsign-conversion"
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

#include "core.h"
#include "color.h"
#include "format.h"

#if defined(_MSC_VER)
#  pragma warning(pop)
#elif defined(__clang__)
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif


namespace fmt
{

    inline static const text_style k_debug_style = fg(color::medium_sea_green);
    inline static const text_style k_info_style = fg(color::sky_blue);
    inline static const text_style k_warning_style = fg(color::gold);
    inline static const text_style k_error_style = fg(color::orange_red);
    inline static const text_style k_fatal_style = fg(color::red) | emphasis::bold | emphasis::reverse;

    template<typename T>
    FMT_CONSTEXPR auto styled_param(const T& v) -> detail::styled_arg<remove_cvref_t<T>>
    {
        return detail::styled_arg<remove_cvref_t<T>>{v, fg(color::orchid) | emphasis::italic};
    }

    template<typename T>
    FMT_CONSTEXPR auto styled_text(const T& v) -> detail::styled_arg<remove_cvref_t<T>>
    {
        return detail::styled_arg<remove_cvref_t<T>>{v, fg(color::sky_blue)};
    }

    template<typename T>
    FMT_CONSTEXPR auto styled_name(const T& v) -> detail::styled_arg<remove_cvref_t<T>>
    {
        return detail::styled_arg<remove_cvref_t<T>>{v, fg(color::medium_sea_green)};
    }

    template<typename T>
    FMT_CONSTEXPR auto styled_important(const T& v) -> detail::styled_arg<remove_cvref_t<T>>
    {
        return detail::styled_arg<remove_cvref_t<T>>{v, fg(color::light_sea_green)};
    }

    template<typename T>
    FMT_CONSTEXPR auto styled_error(const T& v) -> detail::styled_arg<remove_cvref_t<T>>
    {
        return detail::styled_arg<remove_cvref_t<T>>{v, k_error_style};
    }

    template<typename T>
    FMT_CONSTEXPR auto styled_debug(const T& v) -> detail::styled_arg<remove_cvref_t<T>>
    {
        return detail::styled_arg<remove_cvref_t<T>>{v, k_debug_style};
    }

    template<typename T>
    FMT_CONSTEXPR auto styled_info(const T& v) -> detail::styled_arg<remove_cvref_t<T>>
    {
        return detail::styled_arg<remove_cvref_t<T>>{v, k_info_style};
    }

    template<typename T>
    FMT_CONSTEXPR auto styled_warning(const T& v) -> detail::styled_arg<remove_cvref_t<T>>
    {
        return detail::styled_arg<remove_cvref_t<T>>{v, k_warning_style};
    }

    template<typename T>
    FMT_CONSTEXPR auto styled_fatal(const T& v) -> detail::styled_arg<remove_cvref_t<T>>
    {
        return detail::styled_arg<remove_cvref_t<T>>{v, k_fatal_style};
    }

} // namespace fmt
