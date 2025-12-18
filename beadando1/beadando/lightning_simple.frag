#version 450

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_fragPos;
layout(location = 3) in vec3 in_color;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 1) uniform sampler2D modelTexture;

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

vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);

vec3 calcLight(vec3 lightPos, vec3 lightColor)
{
    vec3 normal = normalize(in_normal);
    vec3 lightDir = normalize(lightPos - in_fragPos);
    vec3 viewDir  = normalize(constants.cameraPosition.xyz - in_fragPos);

    // diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // specular
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = spec * lightColor;

    return diffuse + specular;
}

void main() {
    vec3 albedo = texture(modelTexture, in_uv).rgb;

    vec3 ambient = 0.1 * albedo;

    // light 1 (shadow comes later)
    vec3 light1 = calcLight(constants.light1Position.xyz, lightColor);

    // light 2 (NO shadow)
    vec3 light2 = calcLight(constants.light2Position.xyz, lightColor);

    vec3 result = ambient + (light1 + light2) * albedo;
    out_color = vec4(result, 1.0);

    gl_FragDepth = gl_FragCoord.z;
}
