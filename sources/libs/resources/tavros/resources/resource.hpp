#pragma once

#include <tavros/core/noncopyable.hpp>
#include <tavros/resources/resource_reader.hpp>
#include <tavros/resources/resource_writer.hpp>
#include <tavros/core/string_view.hpp>

namespace tavros::resources
{

    class resource : core::noncopyable
    {
    public:
        virtual ~resource() = default;

        virtual [[nodiscard]] resource_reader* reader() = 0;

        virtual [[nodiscard]] resource_writer* writer() = 0;

        virtual [[nodiscard]] core::string_view path() const = 0;

        virtual [[nodiscard]] bool is_open() const = 0;

        virtual void close() = 0;
    };

} // namespace tavros::resources
