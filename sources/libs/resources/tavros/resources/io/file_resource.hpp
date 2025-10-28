#pragma once

#include <tavros/resources/resource.hpp>
#include <tavros/resources/resource_access.hpp>
#include <tavros/resources/io/file_reader.hpp>
#include <tavros/resources/io/file_writer.hpp>
#include <tavros/core/string.hpp>

namespace tavros::resources
{

    class file_resource : public resource
    {
    public:
        explicit file_resource(core::string_view path, resource_access access);

        ~file_resource() override;

        [[nodiscard]] resource_reader* reader() override;

        [[nodiscard]] resource_writer* writer() override;

        [[nodiscard]] core::string_view path() const override;

        [[nodiscard]] bool is_open() const override;

        void close() override;

    private:
        core::string    m_path;
        file_reader     m_reader;
        file_writer     m_writer;
        resource_access m_access;
    };

} // namespace tavros::resources