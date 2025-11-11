#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_normal;

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

void main() { //ebben nagyban segített a chatgpt rossz vagyok matekból meg már annyi transzformáció van itt amúgyis, de ha kell eltudnám mondani nagyjából mit csinál szóval allgud (főleg hogy sokat kellet debuggolni mert gpt fogyi)

    // az első kiinduló csillag pozíció a kristály mellett, majd a phase-l alrébb lesz rakva a többi
    vec3 basepos = vec3(0.0f, 1.0f, 1.0f);

    // Orbit radius multiplier if you want to push them farther
    float radius = 2.5f;
    basepos.xz *= radius; //magasság marad

    //spin around scene's y axis (technikailag a középen levő gyémánt körül)
    // angle = time + phase offset (each instance 90 degrees apart)
    float phase = 3.14159265f * 0.5f * float(gl_InstanceIndex % 4); // PI/2 per instance
    float angle = UBO.time + phase;
    // compute rotation around scene Y axis (apply to XZ)
    float angle_cos = cos(angle);
    float angle_sin = sin(angle);
    // rotate the base position around Y manually:
    // take the X,Z of basePos (centered), rotate them, then add back Y
    vec3 orbitOffset = vec3(
        angle_cos * basepos.x - angle_sin * basepos.z,
        basepos.y + 2.35f,
        angle_sin * basepos.x + angle_cos * basepos.z
    );

    // rotation around *local* Y axis (spin in place)
    float spinSpeed = 2.0f; // how fast each star spins
    float spinAngle = UBO.time * spinSpeed;
    float spinAngle_cos = cos(spinAngle);
    float spinAngle_sin = sin(spinAngle);

    vec3 rotatedPos = vec3(
        spinAngle_cos * in_position.x - spinAngle_sin * in_position.z,
        in_position.y,
        spinAngle_sin * in_position.x + spinAngle_cos * in_position.z
    );

    // --- Final position = local rotation + orbit offset ---
    vec3 current_pos = rotatedPos + orbitOffset;

    //kisebbre scaleljük a modelt számolás előtt
    gl_Position = constants.projection
                * constants.view
                * constants.model
                * vec4(current_pos, 1.0f);

    vec3 gradient = normalize(in_position.xyz) * 0.5f + 0.8f;
    out_color = mix(UBO.color.rgb, gradient, 0.7f);
}
