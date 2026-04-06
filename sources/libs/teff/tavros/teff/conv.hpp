#pragma once

#include <tavros/core/math.hpp>

#include <optional>

namespace tavros::teff
{

    class node;

    template<typename T>
    struct conv
    {
        std::optional<T> operator()(const node* n) const noexcept;
    };

    template<>
    struct conv<math::euler3>
    {
        std::optional<math::euler3> operator()(const node* n) const noexcept;
    };

    template<>
    struct conv<math::mat3>
    {
        std::optional<math::mat3> operator()(const node* n) const noexcept;
    };

    template<>
    struct conv<math::mat4>
    {
        std::optional<math::mat4> operator()(const node* n) const noexcept;
    };

    template<>
    struct conv<math::quat>
    {
        std::optional<math::quat> operator()(const node* n) const noexcept;
    };

    template<>
    struct conv<math::vec2>
    {
        std::optional<math::vec2> operator()(const node* n) const noexcept;
    };

    template<>
    struct conv<math::vec3>
    {
        std::optional<math::vec3> operator()(const node* n) const noexcept;
    };

    template<>
    struct conv<math::vec4>
    {
        std::optional<math::vec4> operator()(const node* n) const noexcept;
    };

    template<>
    struct conv<math::ivec2>
    {
        std::optional<math::ivec2> operator()(const node* n) const noexcept;
    };

    template<>
    struct conv<math::rgba8>
    {
        std::optional<math::rgba8> operator()(const node* n) const noexcept;
    };

} // namespace tavros::teff
