include(CMakeSources.txt)

set_source_files_properties(${TAV_Q3PP_CROSSPLATFORM_SOURCES} PROPERTIES LANGUAGE CXX)

add_executable(
    q3pp
    ${TAV_Q3PP_CROSSPLATFORM_SOURCES}
)

target_link_libraries(
        q3pp
    PRIVATE
        tav_core
        tav_renderer
        tav_system
        glad
        tinyobjloader
        stb
)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set_target_properties(q3pp PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST_FILE ${CMAKE_SOURCE_DIR}/tools/cmake/Info.plist.in
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
endif()

target_include_directories(q3pp PUBLIC ${CMAKE_CURRENT_LIST_DIR}/sources)

target_compile_definitions(q3pp
    PRIVATE
        $<$<CONFIG:Debug>:_CRT_SECURE_NO_WARNINGS>
        $<$<CONFIG:Release>:_CRT_SECURE_NO_WARNINGS>
)

set_target_properties(q3pp PROPERTIES
    VS_DEBUGGER_COMMAND_ARGUMENTS ${CMAKE_SOURCE_DIR}/assets/
)

group_sources_by_folder(q3pp)
