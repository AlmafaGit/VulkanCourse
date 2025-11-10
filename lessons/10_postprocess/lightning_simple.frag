#version 450

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_fragPos;
layout(location = 3) in vec3 in_color;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 1) uniform sampler2D modelTexture;

layout(push_constant) uniform PushConstants {
    // common
    vec3 cameraPosition;
    mat4 projection;
    mat4 view;
    vec4 lightPosition;
    // model spec
    mat4 model;
} constants;

vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);

void main() {
    vec4 pixel = texture(modelTexture, in_uv);

    // ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    // diffuse
    vec3 norm = normalize(in_normal);
    vec3 lightDir = normalize(constants.lightPosition.xyz - in_fragPos);

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(constants.cameraPosition - in_fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

    // combine the colors
    vec3 result = (ambient + diffuse + specular) * pixel.rgb;

    out_color = vec4(result, 1.0);

    gl_FragDepth = gl_FragCoord.z;
}
