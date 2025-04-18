include(CMakeSources.txt)

add_executable(
    tav_sandbox
    ${TAV_SANDBOX_SOURCES}
)

target_link_libraries(
        tav_sandbox
    PRIVATE
        tav_core
)

target_include_directories(tav_sandbox PUBLIC ${CMAKE_CURRENT_LIST_DIR})

group_sources_by_folder(tav_sandbox)
