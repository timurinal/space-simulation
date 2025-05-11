#version 460 core

layout(location = 0) in vec2 aPos;          // (‑0.5 … +0.5)
layout(location = 1) in vec2 aUv;

uniform mat4 view;
uniform mat4 proj;

uniform vec3 spherePosWS;
uniform vec3 bbRight;
uniform vec3 bbUp;
uniform float radius;

out vec2 vUv;

void main()
{
    vUv = aUv;

    vec2 centred = aPos * 2.0;              // (‑1 … +1)
    vec3 offsetWS = (centred.x * bbRight +
    centred.y * bbUp) * radius;

    vec3 worldPos = spherePosWS + offsetWS;
    gl_Position   = proj * view * vec4(worldPos, 1.0);
}
