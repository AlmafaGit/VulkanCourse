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

layout(push_constant) uniform PushConstants {
    // common
    // Camera
    vec4 cameraPosition;
    mat4 projection;
    mat4 view;
    // Light
    vec4 lightPosition;
    // model spec
    mat4 model;
} constants;

vec3 lightColor = vec3(1.0f, 0.0f, 1.0f);

struct {
    float constant;
    float linear;
    float quadratic;
} light = {
//1.0f, 0.09f, 0.032f
0.0f,
0.35f,
0.44f
};


void main() {
    vec4 pixel = texture(modelTexture, in_uv);

    // ambient
    float ambientValue = 0.1f;
    vec3 ambient = lightColor * ambientValue;

    // diffuse
    vec3 normal = normalize(in_normal);
    vec3 lightDir = normalize(constants.lightPosition.xyz - in_fragPos);

    float diff = max(dot(normal, lightDir), 0.0f);
    vec3 diffuse = diff * lightColor;

    // specular
    vec3 viewDir = normalize(constants.cameraPosition.xyz - in_fragPos);
    vec3 reflectionDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectionDir), 0.0f), 32.0f);
    vec3 specular = 1.7f * spec * vec3(1.0f, 1.0f, 1.0f);

    vec3 result = (ambient + diffuse + specular) * pixel.rgb;
    out_color = vec4(pixel.rgb, 1.0f);
}
