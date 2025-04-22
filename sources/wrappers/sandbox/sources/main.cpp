#include <iostream>

#include <tavros/core/prelude.hpp>
#include <tavros/core/timer.hpp>

#include <tavros/core/math.hpp>
#include <tavros/core/geometry/aabb3.hpp>
#include <tavros/core/geometry/sphere.hpp>

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
    tavros::math::quat q(tavros::math::vec3(1.0, 0.0, 0.0), 3.1415 / 2);
    auto               rotated = q.rotate_point(v);

    logger.info("Initial point vec3(): %s", v.to_string(1).c_str());
    logger.info("Rotated point vec3(): %s", rotated.to_string(1).c_str());


    tavros::math::euler3 e(1.12345, 2.34567, 3.45678);

    e = e.normalized();


    auto sph = tavros::geometry::sphere(tavros::math::vec3(10, 10, 10), 5);
    auto v_inside_sph = tavros::math::vec3(9, 9, 9);
    
    auto sph_dist1 = sph.distance(v);
    auto sph_dist2 = sph.distance(v_inside_sph);
    
    logger.info("Dist to sphere1: %f", sph_dist1);
    logger.info("Dist to sphere2: %f", sph_dist2);
    
    tavros::geometry::aabb3 aabb;


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

    aabb.expand(m1.cols[0].xyz);
    aabb.expand(m1.cols[1].xyz);
    aabb.expand(m1.cols[2].xyz);
    aabb.expand(m1.cols[3].xyz);
    aabb.expand(m2.cols[0].xyz);
    aabb.expand(m2.cols[1].xyz);
    aabb.expand(m2.cols[2].xyz);
    aabb.expand(m2.cols[3].xyz);

    logger.info("AABB min: %s", aabb.min.to_string().c_str());
    logger.info("AABB max: %s", aabb.max.to_string().c_str());

    tavros::core::timer mtm;

    mtm.start();
    auto inv1 = m1.inverse();
    auto mat1_inv_time = mtm.elapsed<ns>();

    logger.info("Mat1 inverse() ns time: %" PRIu64, mat1_inv_time);

    mtm.start();
    auto inv2 = m2.inverse();
    auto mat2_inv_time = mtm.elapsed<ns>();

    logger.info("Mat2 inverse() ns time: %" PRIu64, mat2_inv_time);

    auto res1 = m1 * inv1;

    logger.info("Mat1 * inv1: %s", res1.to_string(5).c_str());

    auto res2 = m2 * inv2;
    logger.info("Mat2 * inv2: %s", res2.to_string(5).c_str());


    constexpr auto identity = tavros::math::mat4::identity();
    auto           is_eq = res2.almost_equal(identity, 1e-5f);

    logger.info("Mat2 is eq to identity: %s", (is_eq ? "Yes" : "No"));


    logger.info("Elapsed ns: %" PRIu64, timer.elapsed<ns>());
    timer.start();

    logger.debug("Debug message");
    logger.info("Info message");
    logger.warning("Warning message");
    logger.error("Error message");
    logger.info("Elapsed ns: %" PRIu64, timer.elapsed<ns>());
    timer.start();

    auto s4 = v4.to_string();
    auto s3 = v3.to_string();
    auto s2 = v2.to_string();
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
