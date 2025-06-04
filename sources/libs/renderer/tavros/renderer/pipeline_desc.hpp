#pragma once

#include <tavros/core/types.hpp>

namespace tavros::renderer
{

    enum class cull_face : uint8
    {
        off,
        front,
        back,
    };

    enum class front_face : uint8
    {
        clockwise,
        counter_clockwise,
    };

    enum class depth_test : uint8
    {
        off,
        less,
        equal,
        greater,
        less_equal,
        greater_equal,
    };

    enum class blend : uint8
    {
        off,
        zero,
        one,
        alpha_rgba,
        screen,
        add,
        mul,
        min,
        max,
    };


    struct pipeline_desc
    {
        cull_face  cull_face = cull_face::off;
        front_face front_face = front_face::clockwise;

        bool  wireframe = false;
        ztest depth_test = depth_test::off;
    };

} // namespace tavros::renderer
