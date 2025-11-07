#version 450

layout(location = 0) in vec3 in_position;

layout(push_constant) uniform PushConstants {
    layout(offset = 4*4*4*0) mat4 projection;
    layout(offset = 4*4*4*1) mat4 view;
    layout(offset = 4*4*4*2) mat4 model;
} constants;

void main() {
    gl_Position = constants.projection
                * constants.view
                * constants.model
                * vec4(in_position, 1.0f);
}
