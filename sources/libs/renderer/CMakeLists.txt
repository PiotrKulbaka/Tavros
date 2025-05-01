include(CMakeSources.txt)

tavros_add_library(
    LIB_NAME
        tav_renderer
    LIB_TYPE
        STATIC
        LIB_CROSSPLATFORM_SOURCES
        ${TAV_RENDERER_SOURCES}
    LIB_MACOS_SOURCES
        ${TAV_RENDERER_MACOS_SOURCES}
    LIB_DEPENDS
        PRIVATE
        tav_core
    LIB_DEFINES
)
