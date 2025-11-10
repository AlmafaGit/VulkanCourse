#version 450

// TASK: get the UV coordinates from the vertex shader and sample a texture

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform UniformBuffer {
    vec4 color;
} UBO;

void main() {
    out_color = UBO.color;
}
