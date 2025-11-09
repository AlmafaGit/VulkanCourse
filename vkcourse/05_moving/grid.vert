#version 450

// TASK: read and pass the UV coordinates

layout(location = 0) in vec3 in_position;

layout(set = 0, binding = 0) uniform UniformBuffer {
    vec4 color;
    float time;
} UBO;

layout(push_constant) uniform PushConstants {
    layout(offset = 4 * 4 * 4 * 0) mat4 projection;
    layout(offset = 4 * 4 * 4 * 1) mat4 view;
    layout(offset = 4 * 4 * 4 * 2) mat4 model;
} constants;

layout(location = 0) out vec2 out_uv;

void main() {
    gl_Position = constants.projection
            * constants.view
            * constants.model
            * vec4(in_position, 1.0f);

    //gl_Position.y += sin(gl_VertexIndex + UBO.time);
}
