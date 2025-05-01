include(CMakeSources.txt)

tavros_add_library(
    LIB_NAME
        tav_core
    LIB_TYPE
        STATIC
    LIB_CROSSPLATFORM_SOURCES
        ${TAV_CORE_SOURCES}
    LIB_DEPENDS
    LIB_DEFINES
)
