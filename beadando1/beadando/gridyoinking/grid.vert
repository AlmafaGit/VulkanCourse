#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_normal;

layout(set = 0, binding = 0) uniform UniformBuffer {
    vec4 color;
    float time;
} UBO;

layout(push_constant) uniform PushConstants {
    // common
    vec3 cameraPosition;
    mat4 projection;
    mat4 view;
    vec4 lightPosition;
    // model spec
    mat4 model;
} constants;

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_fragPos;
// TASK: Emit normal and fragPos

void main() {
    gl_Position = constants.projection
            * constants.view
            * constants.model
            * vec4(in_position, 1.0f);

    out_uv = in_uv;
    out_uv.y = 1.0 - in_uv.y;

    // TASK: emit normal and frags
    out_normal = mat3(transpose(inverse(constants.model))) * in_normal;
    out_fragPos = vec3(constants.model * vec4(in_position, 1.0f));
}
