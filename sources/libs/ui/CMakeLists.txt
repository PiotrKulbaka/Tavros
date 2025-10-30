include(CMakeSources.txt)

tavros_add_library(
    LIB_NAME
        tav_ui
    LIB_TYPE
        STATIC
    LIB_CROSSPLATFORM_SOURCES
        ${TAV_UI_CROSSPLATFORM_SOURCES}
    LIB_DEPENDS
        PRIVATE
            tav_core
            tav_renderer
            stb
    LIB_DEFINES
)

set_target_group(tav_ui "libs")
