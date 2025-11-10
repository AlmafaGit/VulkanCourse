#version 450

const vec3 colors[3] = vec3[](
        vec3(1.0f, 0.0f, 0.0f),
        vec3(0.0f, 1.0f, 0.0f),
        vec3(0.0f, 0.0f, 1.0f)
    );

layout(location = 0) in vec3 in_position;

layout(location = 0) out vec3 out_color;

layout(push_constant) uniform PushConstants {
    vec3 cameraPostion;
    mat4 projection;
    mat4 view;
    vec4 lightPosition;
    mat4 model;
} constants;

void main() {
    vec3 current_pos = in_position;

    gl_Position = constants.projection
            * constants.view
            * constants.model
            * vec4(current_pos, 1.0f);

    out_color = colors[gl_VertexIndex % 3];
}
