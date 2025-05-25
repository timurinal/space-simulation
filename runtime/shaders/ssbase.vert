#version 460 core

uniform mat4 proj;
uniform mat4 inv_proj;
uniform mat4 view;
uniform mat4 inv_view;
uniform vec3 cameraPos;

out vec2 vPos;
out vec2 uv;

const vec3 Positions[6] = vec3[6](
    vec3( 1.0,  1.0, 0.0),
    vec3(-1.0, -1.0, 0.0),
    vec3(-1.0,  1.0, 0.0),
    vec3(-1.0, -1.0, 0.0),
    vec3( 1.0,  1.0, 0.0),
    vec3( 1.0, -1.0, 0.0)
);

const vec2 UVs[6] = vec2[6](
    vec2(1.0, 1.0),
    vec2(0.0, 0.0),
    vec2(0.0, 1.0),
    vec2(0.0, 0.0),
    vec2(1.0, 1.0),
    vec2(1.0, 0.0)
);

void main() {
    vec3 p = Positions[gl_VertexID];
    gl_Position = vec4(p, 1.0);

    vPos = p.xy;
    uv = UVs[gl_VertexID];
}