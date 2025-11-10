#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;

layout(set = 0, binding = 0) uniform UniformBuffer {
    vec4 color;
    float time;
} UBO;

layout(push_constant) uniform PushConstants {
    layout(offset = 4*4*4*0) mat4 projection;
    layout(offset = 4*4*4*1) mat4 view;
    layout(offset = 4*4*4*2) mat4 model;
} constants;

layout(location = 0) out vec3 out_color;

void main() {
    vec3 current_pos = in_position;

    gl_Position = constants.projection
    * constants.view
    * constants.model
    * vec4(current_pos, 1.0f);

    //TODO
    vec3 gradient = normalize(in_position.xyz) * 0.5 + 0.5;
    out_color = mix(UBO.color.rgb, gradient, 0.4);

}
