#include <iostream>

#include <tavros/core/prelude.hpp>
#include <tavros/core/timer.hpp>

#include <inttypes.h>

int main()
{
    namespace core = tavros::core;

    tavros::core::logger::add_consumer([](core::severity_level, core::string_view tag, core::string_view msg) {
        TAV_ASSERT(tag.data());
        std::cout << msg << std::endl;
    });

    tavros::core::timer  t;
    tavros::core::logger logger("main");
    uint64_t             us = t.elapsed<std::chrono::nanoseconds>();

    auto n = t.elapsed<std::chrono::nanoseconds>();
    auto n2 = t.elapsed<std::chrono::nanoseconds>();
    auto r = n2 - n;

    logger.info("Elapsed time to create a logger:  %" PRIu64 " ", r);
    t.start();
    logger.debug("Debug message");
    //    logger.info("Info message");
    //    logger.warning("Warning message");
    //    logger.error("Error message");
    us = t.elapsed<std::chrono::nanoseconds>();
    logger.info("Elapsed time to write 4 messages: %" PRIu64 " ", us);

    return 0;
}
