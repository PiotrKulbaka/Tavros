#pragma once

#include <tavros/core/logger/severity_level.hpp>
#include <tavros/core/string.hpp>
#include <tavros/core/string_view.hpp>

#include <functional>

#include <fmt/fmt.hpp>

namespace tavros::core
{

    /**
     * @brief Thread-safe logger with printf-style formatting and multiple consumer support.
     *
     * The logger allows printing messages with different severity levels, tagging,
     * and customizable output consumers. Supports formatting via {fmt} library
     * and ANSI styling for colored output.
     */
    class logger
    {
    public:
        /**
         * @brief Type of consumer function that receives log messages.
         * @param level Severity level of the message.
         * @param tag Tag associated with the logger or message.
         * @param msg Formatted log message string.
         */
        using consumer_type = std::function<void(severity_level, string_view, string_view)>;

        inline static const fmt::text_style k_debug_style = fmt::fg(fmt::color::medium_sea_green);
        inline static const fmt::text_style k_info_style = fmt::fg(fmt::color::sky_blue);
        inline static const fmt::text_style k_warning_style = fmt::fg(fmt::color::gold);
        inline static const fmt::text_style k_error_style = fmt::fg(fmt::color::orange_red);
        inline static const fmt::text_style k_fatal_style = fmt::fg(fmt::color::red) | fmt::emphasis::bold | fmt::emphasis::reverse;

    public:
        /**
         * @brief Constructs a logger with a given tag.
         * @param tag Tag associated with this logger instance.
         */
        explicit logger(string_view tag)
            : m_tag(tag)
        {
        }

        ~logger() noexcept = default;

        /**
         * @brief Logs a debug message with printf-style formatting.
         */
        template<typename... Args>
        void debug(fmt::format_string<Args...> fmt, Args&&... args) const noexcept
        {
            make_message(severity_level::debug, m_tag, fmt, std::forward<Args>(args)...);
        }

        /**
         * @brief Logs an info message with printf-style formatting.
         */
        template<typename... Args>
        void info(fmt::format_string<Args...> fmt, Args&&... args) const noexcept
        {
            make_message(severity_level::info, m_tag, fmt, std::forward<Args>(args)...);
        }

        /**
         * @brief Logs a warning message with printf-style formatting.
         */
        template<typename... Args>
        void warning(fmt::format_string<Args...> fmt, Args&&... args) const noexcept
        {
            make_message(severity_level::warning, m_tag, fmt, std::forward<Args>(args)...);
        }

        /**
         * @brief Logs an error message with printf-style formatting.
         */
        template<typename... Args>
        void error(fmt::format_string<Args...> fmt, Args&&... args) const noexcept
        {
            make_message(severity_level::error, m_tag, fmt, std::forward<Args>(args)...);
        }

        /**
         * @brief Logs a fatal message with printf-style formatting.
         */
        template<typename... Args>
        void fatal(fmt::format_string<Args...> fmt, Args&&... args) const noexcept
        {
            make_message(severity_level::fatal, m_tag, fmt, std::forward<Args>(args)...);
        }

        /**
         * @brief Returns the tag associated with this logger instance.
         */
        string_view tag() const noexcept
        {
            return m_tag;
        }

    public:
        /**
         * @brief Logs a message with a specified severity and tag.
         */
        template<typename... Args>
        static void print(severity_level level, string_view tag, fmt::format_string<Args...> fmt, Args&&... args) noexcept
        {
            make_message(level, tag, fmt, std::forward<Args>(args)...);
        }

        /**
         * @brief Sets the global severity level. Messages below this level will be ignored.
         */
        static void set_severity_level(severity_level level) noexcept;

        /**
         * @brief Adds a consumer callback that will receive log messages.
         */
        static void add_consumer(const consumer_type& consumer) noexcept;

    private:
        template<typename... Args>
        static void make_message(severity_level level, string_view tag, fmt::format_string<Args...> fmt, Args&&... args) noexcept
        {
            if (!allowed_print(level)) {
                return;
            }

            static constexpr size_t buf_size = 4096;
            char                    buf[buf_size];

            string_view     lvl_s;
            fmt::text_style lvl_style;
            auto            now = now_time();

            switch (level) {
            case severity_level::debug:
                lvl_s = "debug";
                lvl_style = k_debug_style;
                break;
            case severity_level::info:
                lvl_s = "info";
                lvl_style = k_info_style;
                break;
            case severity_level::warning:
                lvl_s = "warning";
                lvl_style = k_warning_style;
                break;
            case severity_level::error:
                lvl_s = "error";
                lvl_style = k_error_style;
                break;
            case severity_level::fatal:
                lvl_s = "fatal";
                lvl_style = k_fatal_style;
                break;
            }

            auto prefix_result = fmt::format_to_n(buf, buf_size - 1, "[{}|{}|{}][{}] ", fmt::styled("tavros", fmt::fg(fmt::color::medium_sea_green) | fmt::emphasis::italic), fmt::styled(now, fmt::fg(fmt::color::dark_sea_green) | fmt::emphasis::italic), fmt::styled(lvl_s, lvl_style), fmt::styled(tag, fmt::fg(fmt::color::dark_sea_green)));

            size_t remaining = buf_size - 1 - static_cast<size_t>(prefix_result.out - buf);
            auto   result = fmt::format_to_n(prefix_result.out, remaining, fmt, std::forward<Args>(args)...);

            size_t total_size = static_cast<size_t>(result.out - buf);
            buf[total_size] = 0;

            print_message(level, tag, string_view(buf, total_size));
        }

        static bool allowed_print(severity_level level) noexcept;

        static string_view now_time() noexcept;

        static void print_message(severity_level level, string_view tag, string_view msg) noexcept;

    private:
        string m_tag;
    };

} // namespace tavros::core

namespace fmt
{
    template<typename T>
    FMT_CONSTEXPR auto styled_param(const T& v) -> detail::styled_arg<remove_cvref_t<T>>
    {
        return detail::styled_arg<remove_cvref_t<T>>{v, fmt::fg(fmt::color::orchid) | fmt::emphasis::italic};
    }

    template<typename T>
    FMT_CONSTEXPR auto styled_text(const T& v) -> detail::styled_arg<remove_cvref_t<T>>
    {
        return detail::styled_arg<remove_cvref_t<T>>{v, fmt::fg(fmt::color::sky_blue)};
    }

    template<typename T>
    FMT_CONSTEXPR auto styled_name(const T& v) -> detail::styled_arg<remove_cvref_t<T>>
    {
        return detail::styled_arg<remove_cvref_t<T>>{v, fmt::fg(fmt::color::medium_sea_green)};
    }

    template<typename T>
    FMT_CONSTEXPR auto styled_important(const T& v) -> detail::styled_arg<remove_cvref_t<T>>
    {
        return detail::styled_arg<remove_cvref_t<T>>{v, fmt::fg(fmt::color::light_sea_green)};
    }

    template<typename T>
    FMT_CONSTEXPR auto styled_error(const T& v) -> detail::styled_arg<remove_cvref_t<T>>
    {
        return detail::styled_arg<remove_cvref_t<T>>{v, tavros::core::logger::k_error_style};
    }
} // namespace fmt
