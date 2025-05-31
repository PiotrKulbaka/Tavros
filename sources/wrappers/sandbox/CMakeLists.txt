include(CMakeSources.txt)

add_executable(
    sandbox
    ${TAV_SANDBOX_CROSSPLATFORM_SOURCES}
)

target_link_libraries(
        sandbox
    PRIVATE
        tav_core
        tav_renderer
        tav_system
        glad
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

group_sources_by_folder(sandbox)
