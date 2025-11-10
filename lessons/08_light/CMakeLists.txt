add_executable(08_light
    08_light.cpp

    simple_cube.cpp
    simple_cube.h
    grid.cpp
    grid.h
)

target_include_directories(08_light
    PRIVATE ${Vulkan_INCLUDE_DIRS} ${glfw_INCLUDE_DIRS} ${EXTERNALS_BUILD_DIR}
)

target_link_libraries(08_light
    PRIVATE glfw Vulkan::Vulkan imgui vkcourse
)

add_shaders(08_light
    triangle_in.vert SPV_triangle_in_vert
    triangle_in.frag SPV_triangle_in_frag
    grid.vert SPV_grid_vert
    grid.frag SPV_grid_frag
)
