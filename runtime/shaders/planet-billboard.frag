#version 460 core
layout(depth_any) out float gl_FragDepth;   // keeps early‑Z optimisations if driver supports it

in  vec2 vUv;

uniform vec3 spherePosWS;
uniform float radius;

uniform vec3 bbRight;
uniform vec3 bbUp;
uniform vec3 bbForward;      // (view direction)

uniform vec3 lightPosWS;
uniform vec3 lightColour;

uniform mat4 view;
uniform mat4 proj;

out vec3 fragColour;

void main()
{
    /* ----- reconstruct disc coordinates ----- */
    vec2 centred = vUv * 2.0 - 1.0;
    float r2 = dot(centred, centred);
    if (r2 > 1.0) { discard; }                     // outside the sphere

    float z = sqrt(1.0 - r2);

    /* ----- world‑space normal (as before) ----- */
    vec3 normalWS = normalize( centred.x * bbRight +
    centred.y * bbUp +
    z          * bbForward );

    /* ----- NEW: world‑space fragment position ----- */
    vec3 fragPosWS = spherePosWS +
    (centred.x * bbRight +
    centred.y * bbUp +
    z          * bbForward) * radius;       // ★

    /* ----- point‑light shading ----- */
    vec3  L      = lightPosWS - fragPosWS;
    float dist   = length(L);
    L            = L / dist;                                   // normalise
    float atten  = 1.0 / (dist * dist);                       // 1/r² fall‑off

    float NdotL  = max(dot(normalWS, L), 0.4);
    vec3  diffuse= NdotL * lightColour * (normalWS * 0.5 + 0.5);

    fragColour   = diffuse;   // add ambient / specular as desired

    vec4 clipPos = proj * view * vec4(fragPosWS, 1.0);
    float ndcDepth = clipPos.z / clipPos.w;          // −1 … +1
    gl_FragDepth  = ndcDepth * 0.5 + 0.5;            //  0 …  1
}
