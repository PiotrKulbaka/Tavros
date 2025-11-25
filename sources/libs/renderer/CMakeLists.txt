include(CMakeSources.txt)

tavros_add_library(
    LIB_NAME
        tav_renderer
    LIB_TYPE
        STATIC
    LIB_CROSSPLATFORM_SOURCES
        ${TAV_RENDERER_CROSSPLATFORM_SOURCES}
    LIB_MACOS_SOURCES
        ${TAV_RENDERER_MACOS_SOURCES}
    LIB_WIN32_SOURCES
        ${TAV_RENDERER_WIN32_SOURCES}
    LIB_DEPENDS
        PUBLIC
            tav_text
        PRIVATE
            tav_core
            glad
    LIB_DEFINES
)

set_target_group(tav_renderer "libs")
