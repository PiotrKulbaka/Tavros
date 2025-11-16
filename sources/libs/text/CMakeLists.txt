include(CMakeSources.txt)

tavros_add_library(
    LIB_NAME
        tav_text
    LIB_TYPE
        STATIC
    LIB_CROSSPLATFORM_SOURCES
        ${TAV_TEXT_CROSSPLATFORM_SOURCES}
    LIB_DEPENDS
        PRIVATE
            tav_core
            stb
    LIB_DEFINES
)

set_target_group(tav_text "libs")
