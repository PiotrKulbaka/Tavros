include(CMakeSources.txt)

add_executable(
    sandbox
    ${TAV_SANDBOX_CROSSPLATFORM_SOURCES}
)

target_link_libraries(
        sandbox
    PRIVATE
        tav_core
        tav_input
        tav_renderer
        tav_system
        tav_resources
        tav_ui
        tav_text
        tinyobjloader
        stb
)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set_target_properties(sandbox PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST_FILE ${CMAKE_SOURCE_DIR}/tools/cmake/Info.plist.in
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
endif()

target_include_directories(sandbox PUBLIC ${CMAKE_CURRENT_LIST_DIR})

set(ASSETS_PATH "${CMAKE_SOURCE_DIR}/assets")
set(OUTPUT_PATH "${CMAKE_SOURCE_DIR}/output")

target_compile_definitions(sandbox
    PRIVATE
        TAV_ASSETS_PATH=\"${ASSETS_PATH}\"
        TAV_OUTPUT_PATH=\"${OUTPUT_PATH}\"
)

group_sources_by_folder(sandbox)
