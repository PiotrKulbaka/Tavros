include(CMakeSources.txt)

add_executable(
    tav_sandbox
    ${TAV_SANDBOX_SOURCES}
)

target_link_libraries(
        tav_sandbox
    PRIVATE
        tav_core
        tav_system
)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set_target_properties(tav_sandbox PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST_FILE ${CMAKE_SOURCE_DIR}/tools/cmake/Info.plist.in
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
endif()

target_include_directories(tav_sandbox PUBLIC ${CMAKE_CURRENT_LIST_DIR})

group_sources_by_folder(tav_sandbox)
