add_executable(beadando
        beadando.cpp

        pedestal.cpp
        pedestal.h
        diamond.cpp
        diamond.h
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
)
