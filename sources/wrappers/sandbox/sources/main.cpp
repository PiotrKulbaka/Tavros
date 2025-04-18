#include <iostream>

#include <tavros/core/prelude.hpp>

int main()
{
    namespace core = tavros::core;

    tavros::core::logger::add_consumer([](core::severity_level, core::string_view tag, core::string_view msg) {
        TAV_ASSERT(tag.data());
        std::cout << msg << std::endl;
    });
    tavros::core::logger logger("main");

    logger.debug("Debug message");
    logger.info("Info message");
    logger.warning("Warning message");
    logger.error("Error message");

    return 0;
}
