include(CMakeSources.txt)

tavros_add_library(
    LIB_NAME
        tav_assets
    LIB_TYPE
        STATIC
    LIB_CROSSPLATFORM_SOURCES
        ${TAV_ASSETS_CROSSPLATFORM_SOURCES}
    LIB_MACOS_SOURCES
    LIB_WIN32_SOURCES
    LIB_DEPENDS
        PRIVATE
            tav_core
            stb
    LIB_DEFINES
)

set_target_group(tav_assets "libs")
