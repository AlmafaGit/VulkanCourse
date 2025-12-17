#version 450

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_fragPos;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform UniformBuffer {
    vec4 color;
    float time;
} UBO;

layout(set = 0, binding = 1) uniform sampler2D modelTexture;

void main() {
    vec4 pixel = texture(modelTexture, in_uv);

    vec3 result = pixel.rgb;
    out_color = vec4(result, 1.0f);
}
