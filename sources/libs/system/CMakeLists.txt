include(CMakeSources.txt)

tavros_add_library(
    LIB_NAME
        tav_system
    LIB_TYPE
        STATIC
    LIB_CROSSPLATFORM_SOURCES
        ${TAV_SYSTEM_SOURCES}
    LIB_MACOS_SOURCES
        ${TAV_SYSTEM_MACOS_SOURCES}
    LIB_WIN32_SOURCES
    LIB_DEPENDS
        PRIVATE
            tav_core
    LIB_DEFINES
)


