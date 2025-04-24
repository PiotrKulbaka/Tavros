#include <iostream>

#include <tavros/core/prelude.hpp>
#include <tavros/core/timer.hpp>

#include <tavros/core/math.hpp>
#include <tavros/core/math/utils/make_string.hpp>

#include <inttypes.h>

int main()
{
    namespace core = tavros::core;

    tavros::core::logger::add_consumer([](core::severity_level, core::string_view tag, core::string_view msg) {
        TAV_ASSERT(tag.data());
        std::cout << msg << std::endl;
    });

    tavros::core::logger logger("main");

    using ns = std::chrono::nanoseconds;

    tavros::core::timer total;
    tavros::core::timer timer;
    tavros::math::vec4  v4(1.12345, 2.34567, 3.45678, 4.56789);
    tavros::math::vec3  v3(1.12345, 2.34567, 3.45678);
    tavros::math::vec2  v2(1.12345, 2.34567);


    tavros::math::vec3 v(0, 100, 100);
    tavros::math::quat q = tavros::math::quat::from_axis_angle(tavros::math::vec3(1.0, 0.0, 0.0), 3.1415 / 2);

    auto m1 = tavros::math::mat4(
        {0.09500612330541536, 0.4787112903697103, 0.7323588693174977, 0.531766923237322},
        {0.6136636791506301, 0.8002516623157738, 0.4418533379788342, 0.7885312335732534},
        {0.6476863058572588, 0.22150741556839992, 0.31433848518567953, 0.3980208059969673},
        {0.23435979645828742, 0.29339171629807215, 0.8628252796693925, 0.7352657093708}
    );
    auto m2 = tavros::math::mat4(
        {0.8296404342071727, 0.7177920167661848, 0.8981249005247213, 0.054310683631361045},
        {0.4084198072584456, 0.987232953524218, 0.6201229295156211, 0.7112177673261363},
        {0.8310086921945496, 0.12320011246585638, 0.4066032555957193, 0.17018108655245479},
        {0.9896494206550167, 0.6357118323726472, 0.7001380882169104, 0.42994401718657616}
    );

    auto ax = q.axis();

    logger.info("Axis: %s", tavros::core::make_string(ax, 3).c_str());

    tavros::core::timer mtm;

    mtm.start();
    auto inv1 = tavros::math::inverse(m1);
    auto mat1_inv_time = mtm.elapsed<ns>();

    logger.info("Mat1 inverse() ns time: %" PRIu64, mat1_inv_time);

    mtm.start();
    auto inv2 = tavros::math::inverse(m2);
    auto mat2_inv_time = mtm.elapsed<ns>();

    logger.info("Mat2 inverse() ns time: %" PRIu64, mat2_inv_time);

    auto res1 = m1 * inv1;

    logger.info("Mat1 * inv1: %s", tavros::core::make_string(res1).c_str());

    auto res2 = m2 * inv2;
    logger.info("Mat2 * inv2: %s", tavros::core::make_string(res2).c_str());


    constexpr auto identity = tavros::math::mat4::identity();
    auto           is_eq = tavros::math::almost_equal(res2, identity, 1e-5f);

    logger.info("Mat2 is eq to identity: %s", (is_eq ? "Yes" : "No"));


    logger.info("Elapsed ns: %" PRIu64, timer.elapsed<ns>());
    timer.start();

    logger.debug("Debug message");
    logger.info("Info message");
    logger.warning("Warning message");
    logger.error("Error message");
    logger.info("Elapsed ns: %" PRIu64, timer.elapsed<ns>());
    timer.start();

    auto s4 = tavros::core::make_string(v4);
    auto s3 = tavros::core::make_string(v3);
    auto s2 = tavros::core::make_string(v2);
    logger.info("Elapsed ns: %" PRIu64, timer.elapsed<ns>());
    timer.start();

    logger.debug("vec4: %s", s4.c_str());
    logger.debug("vec3: %s", s3.c_str());
    logger.debug("vec2: %s", s2.c_str());

    logger.info("Elapsed ns: %" PRIu64, timer.elapsed<ns>());
    timer.start();

    logger.info("Elapsed ns: %" PRIu64, timer.elapsed<ns>());


    logger.info("Total ns: %" PRIu64, total.elapsed<ns>());

    return 0;
}
