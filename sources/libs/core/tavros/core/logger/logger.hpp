#pragma once

#include <tavros/core/logger/severity_level.hpp>
#include <tavros/core/defines.hpp>
#include <tavros/core/string.hpp>
#include <tavros/core/string_view.hpp>

#include <cstdarg>
#include <functional>

#if TAV_COMPILER_GCC
    #define TAV_FORMAT_PRINTF(string_index, first_to_check) \
        __attribute__((format(printf, string_index, first_to_check)))
#else
    #define TAV_FORMAT_PRINTF(string_index, first_to_check)
#endif

namespace tavros::core
{

    /**
     * @brief Simple multithreaded logger class with printf-style logging and multiple consumers support
     */
    class logger
    {
    public:
        /**
         * @brief Logger consumer function type
         */
        using consumer_type = std::function<void(severity_level, string_view, string_view)>;

    public:
        /**
         * @brief Logger constructor
         */
        explicit logger(string_view tag);

        /**
         * @brief Printf-style log debug message
         */
        void debug(const char* fmt, ...) TAV_FORMAT_PRINTF(2, 3);

        /**
         * @brief Printf-style log info message
         */
        void info(const char* fmt, ...) TAV_FORMAT_PRINTF(2, 3);

        /**
         * @brief Printf-style log warning message
         */
        void warning(const char* fmt, ...) TAV_FORMAT_PRINTF(2, 3);

        /**
         * @brief Printf-style log error message
         */
        void error(const char* fmt, ...) TAV_FORMAT_PRINTF(2, 3);

        /**
         * @brief The logger tag
         */
        string_view tag() const;

    public:
        /**
         * @brief Print printf-style message with specified severity level
         */
        static void print(severity_level level, const char* tag, const char* fmt, ...) TAV_FORMAT_PRINTF(3, 4);

        /**
         * @brief Print vprintf-style message with specified severity level
         */
        static void vprint(severity_level level, const char* tag, const char* fmt, va_list args);

        /**
         * @brief Set logger severity level
         */
        static void set_severity_level(severity_level level);


        /**
         * @brief Add logger consumer
         */
        static void add_consumer(const consumer_type& consumer);

    private:
        string m_tag;
    };

} // namespace tavros::core
