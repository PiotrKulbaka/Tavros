#pragma once

#include <tavros/core/math.hpp>
#include <tavros/core/string_view.hpp>
#include <tavros/core/string.hpp>

#include <optional>

namespace tavros::tef
{

    class node;

    template<typename T>
    struct conv
    {
        std::optional<T> read(const node* n) const noexcept;

        void write(node* parent, core::string_view key, const T& val) const;

        core::string_view description() const noexcept;
    };

    template<>
    struct conv<bool>
    {
        std::optional<bool> read(const node* n) const noexcept;

        void write(node* parent, core::string_view key, const bool& val) const;

        core::string_view description() const noexcept;
    };

    template<>
    struct conv<uint8>
    {
        std::optional<uint8> read(const node* n) const noexcept;

        void write(node* parent, core::string_view key, const uint8& val) const;

        core::string_view description() const noexcept;
    };

    template<>
    struct conv<int8>
    {
        std::optional<int8> read(const node* n) const noexcept;

        void write(node* parent, core::string_view key, const int8& val) const;

        core::string_view description() const noexcept;
    };

    template<>
    struct conv<uint16>
    {
        std::optional<uint16> read(const node* n) const noexcept;

        void write(node* parent, core::string_view key, const uint16& val) const;

        core::string_view description() const noexcept;
    };

    template<>
    struct conv<int16>
    {
        std::optional<int16> read(const node* n) const noexcept;

        void write(node* parent, core::string_view key, const int16& val) const;

        core::string_view description() const noexcept;
    };

    template<>
    struct conv<uint32>
    {
        std::optional<uint32> read(const node* n) const noexcept;

        void write(node* parent, core::string_view key, const uint32& val) const;

        core::string_view description() const noexcept;
    };

    template<>
    struct conv<int32>
    {
        std::optional<int32> read(const node* n) const noexcept;

        void write(node* parent, core::string_view key, const int32& val) const;

        core::string_view description() const noexcept;
    };

    template<>
    struct conv<uint64>
    {
        std::optional<uint64> read(const node* n) const noexcept;

        void write(node* parent, core::string_view key, const uint64& val) const;

        core::string_view description() const noexcept;
    };

    template<>
    struct conv<int64>
    {
        std::optional<int64> read(const node* n) const noexcept;

        void write(node* parent, core::string_view key, const int64& val) const;

        core::string_view description() const noexcept;
    };

    template<>
    struct conv<float>
    {
        std::optional<float> read(const node* n) const noexcept;

        void write(node* parent, core::string_view key, const float& val) const;

        core::string_view description() const noexcept;
    };

    template<>
    struct conv<double>
    {
        std::optional<double> read(const node* n) const noexcept;

        void write(node* parent, core::string_view key, const double& val) const;

        core::string_view description() const noexcept;
    };

    template<>
    struct conv<core::string_view>
    {
        std::optional<core::string_view> read(const node* n) const noexcept;

        void write(node* parent, core::string_view key, const core::string_view& val) const;

        core::string_view description() const noexcept;
    };

    template<>
    struct conv<core::string>
    {
        std::optional<core::string> read(const node* n) const noexcept;

        void write(node* parent, core::string_view key, const core::string& val) const;

        core::string_view description() const noexcept;
    };

    template<>
    struct conv<math::euler3>
    {
        std::optional<math::euler3> read(const node* n) const noexcept;

        void write(node* parent, core::string_view key, const math::euler3& val) const;

        core::string_view description() const noexcept;
    };

    template<>
    struct conv<math::vec2>
    {
        std::optional<math::vec2> read(const node* n) const noexcept;

        void write(node* parent, core::string_view key, const math::vec2& val) const;

        core::string_view description() const noexcept;
    };

    template<>
    struct conv<math::vec3>
    {
        std::optional<math::vec3> read(const node* n) const noexcept;

        void write(node* parent, core::string_view key, const math::vec3& val) const;

        core::string_view description() const noexcept;
    };

    template<>
    struct conv<math::vec4>
    {
        std::optional<math::vec4> read(const node* n) const noexcept;

        void write(node* parent, core::string_view key, const math::vec4& val) const;

        core::string_view description() const noexcept;
    };

    template<>
    struct conv<math::quat>
    {
        std::optional<math::quat> read(const node* n) const noexcept;

        void write(node* parent, core::string_view key, const math::quat& val) const;

        core::string_view description() const noexcept;
    };

    template<>
    struct conv<math::ivec2>
    {
        std::optional<math::ivec2> read(const node* n) const noexcept;

        void write(node* parent, core::string_view key, const math::ivec2& val) const;

        core::string_view description() const noexcept;
    };

    template<>
    struct conv<math::rgba8>
    {
        std::optional<math::rgba8> read(const node* n) const noexcept;

        void write(node* parent, core::string_view key, const math::rgba8& val) const;

        core::string_view description() const noexcept;
    };

    template<>
    struct conv<math::mat3>
    {
        std::optional<math::mat3> read(const node* n) const noexcept;

        void write(node* parent, core::string_view key, const math::mat3& val) const;

        core::string_view description() const noexcept;
    };

    template<>
    struct conv<math::mat4>
    {
        std::optional<math::mat4> read(const node* n) const noexcept;

        void write(node* parent, core::string_view key, const math::mat4& val) const;

        core::string_view description() const noexcept;
    };

} // namespace tavros::tef
