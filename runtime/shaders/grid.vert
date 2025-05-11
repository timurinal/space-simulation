// Credit for the grid shader goes to
//      https://asliceofrendering.com/scene%20helper/2020/01/05/InfiniteGrid/

#version 460 core

uniform mat4 proj;
uniform mat4 inv_proj;
uniform mat4 view;
uniform mat4 inv_view;
uniform vec3 cameraPos;

out vec3 nearPoint;
out vec3 farPoint;

const vec3 Positions[6] = vec3[6](
    vec3( 1.0,  1.0, 0.0),
    vec3(-1.0, -1.0, 0.0),
    vec3(-1.0,  1.0, 0.0),
    vec3(-1.0, -1.0, 0.0),
    vec3( 1.0,  1.0, 0.0),
    vec3( 1.0, -1.0, 0.0)
);

vec3 unprojectPoint(vec3 point) {
    vec4 unprojectedPoint = inv_view * inv_proj * vec4(point, 1.0);
    return unprojectedPoint.xyz / unprojectedPoint.w;
}

void main() {
    vec3 p = Positions[gl_VertexID];
    nearPoint = unprojectPoint(vec3(p.xy, 0.0));
    farPoint = unprojectPoint(vec3(p.xy, 1.0));
    gl_Position = vec4(p, 1.0);
}