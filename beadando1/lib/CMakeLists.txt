set(NAME vkcourse)
add_library(${NAME} STATIC
    buffer.cpp
    descriptors.cpp
    texture.cpp

    context.cpp
    swapchain.cpp
    imgui_integration.cpp
    wrappers.cpp
)

target_include_directories(${NAME}
    PUBLIC ${Vulkan_INCLUDE_DIRS} ${EXTERNALS_BUILD_DIR} ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${NAME}
    PUBLIC Vulkan::Vulkan stb imgui
)
