include(CMakeSources.txt)

tavros_add_library(
    LIB_NAME
        tav_core
    LIB_TYPE
        STATIC
    LIB_CROSSPLATFORM_SOURCES
        ${TAV_CORE_CROSSPLATFORM_SOURCES}
    LIB_DEPENDS
        PUBLIC
            fmt
    LIB_DEFINES
)
