#pragma once

#include <tavros/core/fixed_string.hpp>

#include <fmt/fmt.hpp>

namespace tavros::core
{

    template<size_t MaxSize>
    class basic_diagnostics final
    {
    public:
        basic_diagnostics() noexcept = default;
        ~basic_diagnostics() noexcept = default;

        [[nodiscard]] uint32 debug_count() const noexcept
        {
            return m_debug_count;
        }

        [[nodiscard]] uint32 info_count() const noexcept
        {
            return m_info_count;
        }

        [[nodiscard]] uint32 warning_count() const noexcept
        {
            return m_warning_count;
        }

        [[nodiscard]] uint32 error_count() const noexcept
        {
            return m_error_count;
        }

        [[nodiscard]] uint32 fatal_count() const noexcept
        {
            return m_fatal_count;
        }

        [[nodiscard]] uint32 total_count() const noexcept
        {
            return m_debug_count + m_info_count + m_warning_count + m_error_count + m_fatal_count;
        }

        template<class... Args>
        void debug(fmt::format_string<Args...> fmt, Args&&... args) noexcept
        {
            ++m_debug_count;
            m_buffer.fprint("Debug: ");
            m_buffer.fprintln(fmt, std::forward<Args>(args)...);
        }

        template<class... Args>
        void info(fmt::format_string<Args...> fmt, Args&&... args) noexcept
        {
            ++m_info_count;
            m_buffer.fprint("Info: ");
            m_buffer.fprintln(fmt, std::forward<Args>(args)...);
        }

        template<class... Args>
        void warning(fmt::format_string<Args...> fmt, Args&&... args) noexcept
        {
            ++m_warning_count;
            m_buffer.fprint("Warning: ");
            m_buffer.fprintln(fmt, std::forward<Args>(args)...);
        }

        template<class... Args>
        void error(fmt::format_string<Args...> fmt, Args&&... args) noexcept
        {
            ++m_error_count;
            m_buffer.fprint("Error: ");
            m_buffer.fprintln(fmt, std::forward<Args>(args)...);
        }

        template<class... Args>
        void fatal(fmt::format_string<Args...> fmt, Args&&... args) noexcept
        {
            ++m_fatal_count;
            m_buffer.fprint("Fatal: ");
            m_buffer.fprintln(fmt, std::forward<Args>(args)...);
        }

        [[nodiscard]] core::string_view text() const noexcept
        {
            return m_buffer;
        }

        void clear() noexcept
        {
            m_buffer.clear();
            m_debug_count = 0;
            m_info_count = 0;
            m_warning_count = 0;
            m_error_count = 0;
            m_fatal_count = 0;
        }

    private:
        uint32 m_debug_count = 0;
        uint32 m_info_count = 0;
        uint32 m_warning_count = 0;
        uint32 m_error_count = 0;
        uint32 m_fatal_count = 0;

        fixed_string<MaxSize> m_buffer;
    };

    using diagnostics = basic_diagnostics<16_kib>;
} // namespace tavros::core
