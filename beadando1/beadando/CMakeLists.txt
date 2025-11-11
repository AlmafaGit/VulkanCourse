add_executable(beadando
        beadando.cpp

        pedestal.cpp
        pedestal.h
        crystal.cpp
        crystal.h
        star.cpp
        star.h
)

target_include_directories(beadando
    PRIVATE ${Vulkan_INCLUDE_DIRS} ${glfw_INCLUDE_DIRS} ${EXTERNALS_BUILD_DIR}
)

target_link_libraries(beadando
    PRIVATE glfw Vulkan::Vulkan imgui vkcourse
)

add_shaders(beadando
        triangle_in.vert SPV_triangle_in_vert
        triangle_in.frag SPV_triangle_in_frag
        crystal.vert SPV_crystal_vert
        crystal.frag SPV_crystal_frag
        star.vert SPV_star_vert
        star.frag SPV_star_frag
)
