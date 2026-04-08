include(CMakeSources.txt)

tavros_add_library(
    LIB_NAME
        tav_tef
    LIB_TYPE
        STATIC
    LIB_CROSSPLATFORM_SOURCES
        ${TAV_TEF_CROSSPLATFORM_SOURCES}
    LIB_DEPENDS
        PUBLIC
            tav_core
    LIB_DEFINES
)

set_target_group(tav_tef "libs")
