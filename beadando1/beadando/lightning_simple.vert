#version 450

const vec3 colors[3] = vec3[](
        vec3(1.0f, 0.0f, 0.0f),
        vec3(0.0f, 1.0f, 0.0f),
        vec3(0.0f, 0.0f, 1.0f)
    );

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_normal;

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_fragPos;
layout(location = 3) out vec3 out_color;

layout(push_constant) uniform PushConstants {
    // common
    vec4 cameraPosition;
    mat4 projection;
    mat4 view;
    // Light
    vec4 light1Position;
    vec4 light2Position;
    // model spec
    mat4 model;
} constants;

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

    vec3 current_pos = in_position;

    out_color = colors[gl_VertexIndex % 3];
}
