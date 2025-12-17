add_executable(beadando
        beadando.cpp

        shadow_map.cpp
        lightning_pass.cpp

        pedestal.cpp
        crystal.cpp
        star.cpp

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

add_shader(beadando shadow_map.vert SPV_shadow_map_vert)
add_shader(beadando shadow_map.frag SPV_shadow_map_frag)

add_shader(beadando lightning_simple.vert SPV_lightning_simple_vert)
add_shader(beadando lightning_simple.frag SPV_lightning_simple_frag)
add_shader(beadando lightning_no.frag SPV_lightning_no_frag)

add_shader(beadando lightning_shadowmap.vert SPV_lightning_shadowmap_vert)
add_shader(beadando lightning_shadowmap.frag SPV_lightning_shadowmap_frag)