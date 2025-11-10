add_executable(02_triangle
    02_triangle.cpp
    simple_triangle.cpp
)

target_include_directories(02_triangle
    PRIVATE ${Vulkan_INCLUDE_DIRS} ${glfw_INCLUDE_DIRS}
)

target_link_libraries(02_triangle
    PRIVATE glfw Vulkan::Vulkan imgui vkcourse
)

add_shaders(02_triangle
    triangle.vert SPV_triangle_vert
    triangle.frag SPV_triangle_frag
)

#add_shader(02_triangle triangle.vert SPV_triangle_vert)
#add_shader(02_triangle triangle.frag SPV_triangle_frag)
