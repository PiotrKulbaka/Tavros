function(group_sources_by_folder target)
    get_target_property(SRCS ${target} SOURCES)

    foreach(src ${SRCS})
        if (NOT IS_ABSOLUTE "${src}")
            set(src "${CMAKE_CURRENT_SOURCE_DIR}/${src}")
        endif()

        file(RELATIVE_PATH rel_path "${CMAKE_CURRENT_LIST_DIR}" "${src}")
        get_filename_component(group_path "${rel_path}" PATH)

        if (WIN32)
            string(REPLACE "/" "\\" group "${group_path}")
        else()
            set(group "${group_path}")
        endif()

        source_group("${group}" FILES "${src}")
    endforeach()
endfunction()

function(set_if_not_defined var value)
    if (NOT DEFINED ${var})
        set(${var} ${value} CACHE STRING "Set ${var} if not defined" FORCE)
    endif()
endfunction()

function(set_if_not_defined_bool var value)
    if (NOT DEFINED ${var})
        set(${var} ${value} CACHE BOOL "Set ${var} if not defined" FORCE)
    endif()
endfunction()

function(set_if_not_defined_path var value)
    if (NOT DEFINED ${var})
        set(${var} ${value} CACHE PATH "Set ${var} if not defined" FORCE)
    endif()
endfunction()

function(tavros_add_library)
    set(LIB_NAME "")
    set(LIB_TYPE "STATIC")
    set(LIB_SOURCES "")
    set(LIB_DEFINES "")
    set(PUBLIC_DEPS "")
    set(PRIVATE_DEPS "")
    set(INTERFACE_DEPS "")

    set(current_section "")
    foreach(arg IN LISTS ARGN)
        if(arg STREQUAL "LIB_NAME")
            set(current_section "LIB_NAME")
        elseif(arg STREQUAL "LIB_TYPE")
            set(current_section "LIB_TYPE")
        elseif(arg STREQUAL "LIB_SOURCES")
            set(current_section "LIB_SOURCES")
        elseif(arg STREQUAL "LIB_DEPENDS")
            set(current_section "LIB_DEPENDS")
        elseif(arg STREQUAL "LIB_DEFINES")
            set(current_section "LIB_DEFINES")
        elseif(arg STREQUAL "PUBLIC")
            set(current_section "PUBLIC")
        elseif(arg STREQUAL "PRIVATE")
            set(current_section "PRIVATE")
        elseif(arg STREQUAL "INTERFACE")
            set(current_section "INTERFACE")
        else()
            if(current_section STREQUAL "LIB_NAME")
                set(LIB_NAME "${arg}")
            elseif(current_section STREQUAL "LIB_TYPE")
                set(LIB_TYPE "${arg}")
            elseif(current_section STREQUAL "LIB_SOURCES")
                list(APPEND LIB_SOURCES "${arg}")
            elseif(current_section STREQUAL "LIB_DEFINES")
                list(APPEND LIB_DEFINES "${arg}")
            elseif(current_section STREQUAL "PUBLIC")
                list(APPEND PUBLIC_DEPS "${arg}")
            elseif(current_section STREQUAL "PRIVATE")
                list(APPEND PRIVATE_DEPS "${arg}")
            elseif(current_section STREQUAL "INTERFACE")
                list(APPEND INTERFACE_DEPS "${arg}")
            endif()
        endif()
    endforeach()

    if(LIB_NAME STREQUAL "")
        message(FATAL_ERROR "LIB_NAME must be specified")
    endif()

    if(LIB_SOURCES STREQUAL "")
        message(FATAL_ERROR "No source files specified for ${LIB_NAME}")
    endif()

    add_library(${LIB_NAME} ${LIB_TYPE} ${LIB_SOURCES})
    target_include_directories(${LIB_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})

    if(LIB_DEFINES)
        target_compile_definitions(${LIB_NAME} PUBLIC ${LIB_DEFINES})
    endif()


    if(PUBLIC_DEPS)
        target_link_libraries(${LIB_NAME}
            PUBLIC ${PUBLIC_DEPS}
        )
    endif()

    if(PRIVATE_DEPS)
        target_link_libraries(${LIB_NAME}
            PRIVATE ${PRIVATE_DEPS}
        )
    endif()

    if(INTERFACE_DEPS)
        target_link_libraries(${LIB_NAME}
            INTERFACE ${INTERFACE_DEPS}
        )
    endif()

    if (MSVC)
        target_compile_options(${LIB_NAME} PRIVATE /W4 /permissive- /Zc:__cplusplus)
    else()
        target_compile_options(${LIB_NAME} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    endif()

    group_sources_by_folder(${LIB_NAME})
endfunction()
