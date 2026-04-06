include(CMakeSources.txt)

tavros_add_library(
    LIB_NAME
        tav_teff
    LIB_TYPE
        STATIC
    LIB_CROSSPLATFORM_SOURCES
        ${TAV_TEFF_CROSSPLATFORM_SOURCES}
    LIB_DEPENDS
        PUBLIC
            tav_core
    LIB_DEFINES
)

set_target_group(tav_teff "libs")
