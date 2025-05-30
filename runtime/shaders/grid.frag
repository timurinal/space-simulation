// Credit for the grid shader goes to
//      https://asliceofrendering.com/scene%20helper/2020/01/05/InfiniteGrid/

#version 460 core

out vec4 fragColour;

in vec3 nearPoint;
in vec3 farPoint;

uniform mat4 proj;
uniform mat4 view;

uniform mat4 projFar;

uniform float nearPlane;
uniform float farPlane;

vec4 grid(vec3 fragPos, float scale, bool drawAxisLines) {
    vec2 coord = fragPos.xz * scale;
    vec2 derivative = fwidth(coord);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line = min(grid.x, grid.y);
    float minimumz = min(derivative.y, 1);
    float minimumx = min(derivative.x, 1);
    vec4 colour = vec4(0.15, 0.15, 0.15, 1.0 - min(line, 1.0));

    if (drawAxisLines) {
        // z axis
        if (fragPos.x > -0.5 * minimumx && fragPos.x < 0.5 * minimumx) colour.b = 1.0;

        // x axis
        if (fragPos.z > -0.5 * minimumz && fragPos.z < 0.5 * minimumz) colour.r = 1.0;
    }

    return colour;
}

float computeDepth(vec3 pos) {
    vec4 clipSpacePos = projFar * view * vec4(pos, 1.0);
    return (clipSpacePos.z / clipSpacePos.w) * 0.5 + 0.5;
}

float computeLinearDepth(vec3 pos) {
    vec4 clipSpacePos = proj * view * vec4(pos, 1.0);
    float clipDepth = clipSpacePos.z / clipSpacePos.w;
    float linearDepth = (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - clipDepth * (farPlane - nearPlane));
    return linearDepth / farPlane;
}

void main() {
    float t = -nearPoint.y / (farPoint.y - nearPoint.y);
    vec3 fragPos = nearPoint + t * (farPoint - nearPoint);

    gl_FragDepth = computeDepth(fragPos);

    float linearDepth = computeLinearDepth(fragPos);
    float fading = max(0, 0.5 - linearDepth);

    vec4 grid100m = grid(fragPos, 0.1, false);
    vec4 grid1km = grid(fragPos, 0.01, false);
    vec4 grid10km = grid(fragPos, 0.01, false);
    vec4 axisLines = grid(fragPos, 1.0, true) - grid(fragPos, 1.0, false);

    // Apply the fading and ensure we only draw where t > 0
    fragColour = (grid100m + grid1km + grid10km + axisLines) * float(t > 0);
    fragColour.a *= fading;
}