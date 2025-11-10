
Vertex layout for all models:


layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_normal;

descriptors:

layout(set = 0, binding = 0) uniform UniformBuffer {
    vec4 color;
    float time;
} UBO;


push constatns:

layout(push_constant) uniform PushConstants {
    // common
    vec3 cameraPosition;
    mat4 projection;
    mat4 view;
    vec4 lightPosition;
    // model spec
    mat4 model;
} constants;
