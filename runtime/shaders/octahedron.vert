#version 460 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexcoord;

uniform mat4 worldToClip;

uniform vec3 spherePos;
uniform float radius;

out vec3 normal;
out vec2 uv;
out vec3 fragPos;

void main() {
    vec3 worldPos = aPosition * radius + spherePos;
    gl_Position = worldToClip * vec4(worldPos, 1.0);

    normal = aNormal;
    uv = aTexcoord;
    fragPos = worldPos;
}
