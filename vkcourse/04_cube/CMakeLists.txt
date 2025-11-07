add_executable(04_cube
    04_cube.cpp

    simple_cube.cpp
    simple_cube.h
)

target_include_directories(04_cube
    PRIVATE ${Vulkan_INCLUDE_DIRS} ${glfw_INCLUDE_DIRS} ${EXTERNALS_BUILD_DIR}
)

target_link_libraries(04_cube
    PRIVATE glfw Vulkan::Vulkan imgui vkcourse
)

add_shader(04_cube triangle_in.vert SPV_triangle_in_vert)
add_shader(04_cube triangle_in.frag SPV_triangle_in_frag)
