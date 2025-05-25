#version 460 core

const float INF = 1.0 / 0.0;

out vec3 fragColour;

in vec2 vPos;
in vec2 uv;

uniform sampler2D MainTex;
uniform sampler2D DepthTex;

uniform mat4 inv_proj;
uniform mat4 inv_view;

uniform vec3 cameraPos;

uniform vec2 screenSize;

uniform vec3 sunPosition;

uniform vec3 atmospherePosition;
uniform float atmosphereRadius;
uniform float planetRadius;
uniform int numInScatteringPoints;
uniform int numOpticalDepthPoints;
uniform float densityFalloff;
uniform vec3 scatteringCoefficients;

vec2 raySphere(vec3 sphereCentre, float sphereRadius, vec3 rayOrigin, vec3 rayDir) {
    vec3 offset = rayOrigin - sphereCentre;
    float a = 1;
    float b = 2 * dot(offset, rayDir);
    float c = dot(offset, offset) - sphereRadius * sphereRadius;
    float d = b * b - 4 * a * c;

    if (d > 0) {
        float s = sqrt(d);
        float dstToSphereNear = max(0, (-b - s) / (2 * a));
        float dstToSphereFar = (-b + s) / (2 * a);

        if (dstToSphereFar >= 0) {
            return vec2(dstToSphereNear, dstToSphereFar - dstToSphereNear);
        }
    }

    return vec2(INF, 0);
}

float lineariseDepth(float d, float near, float far)
{
    float z = d * 2.0 - 1.0;           // back to NDC
    return (2.0 * near * far) /
    (far + near - z * (far - near));
}

float densityAtPoint(vec3 densitySamplePoint);
float opticalDepth(vec3 rayOrigin, vec3 rayDir, float rayLength);
vec3 calculateLight(vec3 rayOrigin, vec3 rayDir, float rayLength, vec3 originalCol);

void main() {
    vec4 clipPos = vec4(vPos.xy, 1.0, 1.0);
    vec4 eyePos  = inv_proj * clipPos;
    eyePos /= eyePos.w;  // perspective divide -> now in eye space
    vec4 worldPos = inv_view * eyePos;
    vec3 cameraWorldPos = (inv_view * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    vec3 rayDir = normalize(worldPos.xyz - cameraWorldPos);

    vec3 originalCol = texture(MainTex, uv).rgb;

    float sceneDepthNonLinear = texture(DepthTex, uv).r;
    float sceneDepth = lineariseDepth(sceneDepthNonLinear, 0.1f, 2500.0f);

    vec2 sphereHit = raySphere(atmospherePosition, atmosphereRadius, cameraPos, rayDir);
    float dstToAtmosphere = sphereHit.x;
    float dstThroughAtmosphere = min(sphereHit.y, sceneDepth - dstToAtmosphere);

    if (dstThroughAtmosphere > 0) {
        const float epsilon = 0.0001;
        vec3 pointInAtmosphere = cameraPos + rayDir * (dstToAtmosphere + epsilon);
        vec3 light = calculateLight(pointInAtmosphere, rayDir, dstThroughAtmosphere - epsilon * 2, originalCol);
        fragColour = light;
    } else {
        fragColour = originalCol;
    }
}

float densityAtPoint(vec3 densitySamplePoint) {
    float heightAboveSurface = length(densitySamplePoint - atmospherePosition) - planetRadius;
    float scaleHeight = 10.0;
    float localDensity = exp(-heightAboveSurface / scaleHeight);
    return localDensity;
}

float opticalDepth(vec3 rayOrigin, vec3 rayDir, float rayLength) {
    vec3 densitySamplePoint = rayOrigin;
    float stepSize = rayLength / (numOpticalDepthPoints - 1);
    float opticalDepth = 0;

    for (int i = 0; i < numOpticalDepthPoints; i++) {
        float localDensity = densityAtPoint(densitySamplePoint);
        opticalDepth += localDensity * stepSize;
        densitySamplePoint += rayDir * stepSize;
    }

    return opticalDepth;
}

vec3 calculateLight(vec3 rayOrigin, vec3 rayDir, float rayLength, vec3 originalCol) {
    vec3 inScatterPoint = rayOrigin;
    float stepSize = rayLength / (numInScatteringPoints - 1);
    vec3 inScatteredLight = vec3(0);
    float viewRayOpticalDepth = 0;

    for (int i = 0; i < numInScatteringPoints; i++) {
        vec3 dirToSun = normalize(sunPosition - inScatterPoint);
        float sunRayLength = raySphere(atmospherePosition, atmosphereRadius, inScatterPoint, dirToSun).y;
        float sunRayOpticalDepth = opticalDepth(inScatterPoint, dirToSun, sunRayLength);
        viewRayOpticalDepth = opticalDepth(inScatterPoint, -rayDir, stepSize * i);
        vec3 transmittance = exp(-(sunRayOpticalDepth + viewRayOpticalDepth) * scatteringCoefficients);
        float localDensity = densityAtPoint(inScatterPoint);

        // float selfShadow = raySphere(atmospherePosition, planetRadius, inScatterPoint, dirToSun).x == INF ? 1 : 0;

        inScatteredLight += localDensity * transmittance * scatteringCoefficients * stepSize;
        inScatterPoint += rayDir * stepSize;
    }

    float originalColTransmittance = exp(-viewRayOpticalDepth);
    return originalCol * originalColTransmittance + inScatteredLight;
}