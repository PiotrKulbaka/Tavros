#include <iostream>

#include <tavros/core/prelude.hpp>
#include <tavros/core/timer.hpp>

#include <tavros/core/math/vec4.hpp>
#include <tavros/core/math/vec3.hpp>
#include <tavros/core/math/vec2.hpp>

#include <inttypes.h>

int main()
{
    namespace core = tavros::core;

    tavros::core::logger::add_consumer([](core::severity_level, core::string_view tag, core::string_view msg) {
        TAV_ASSERT(tag.data());
        std::cout << msg << std::endl;
    });

    using ns = std::chrono::nanoseconds;

    tavros::core::timer      total;
    tavros::core::timer      timer;
    tavros::core::math::vec4 v4(1.12345, 2.34567, 3.45678, 4.56789);
    tavros::core::math::vec3 v3(1.12345, 2.34567, 3.45678);
    tavros::core::math::vec2 v2(1.12345, 2.34567);


    tavros::core::logger logger("main");

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
