add_subdirectory(glad)
add_subdirectory(stb)
add_subdirectory(tinyobjloader)
add_subdirectory(fmt)
add_subdirectory(zlib)
add_subdirectory(tracy)
add_subdirectory(tomlplusplus)

set_target_group(glad "third")
set_target_group(stb "third")
set_target_group(tinyobjloader "third")
set_target_group(fmt "third")
set_target_group(zlibstatic "third")
set_target_group(TracyClient "third")
set_target_group(toml "third")

if (MSVC)
    target_compile_options(
        fmt
        PRIVATE
        /wd26495 # uninitialized variable
        /wd26498 # variable is not a constexpr used in a constexpr function
    )
endif()
