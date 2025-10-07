include(CMakeSources.txt)

tavros_add_library(
    LIB_NAME
        tav_resources
    LIB_TYPE
        STATIC
    LIB_CROSSPLATFORM_SOURCES
        ${TAV_RESOURCES_CROSSPLATFORM_SOURCES}
    LIB_MACOS_SOURCES
    LIB_WIN32_SOURCES
    LIB_DEPENDS
        PRIVATE
            tav_core
    LIB_DEFINES
)

set_target_group(tav_resources "libs")
