#version 460 core

struct Material {
    vec3 diffuse;
    int emissive;
    vec4 emission; // alpha is intensity

    int hasTexture;
    sampler2D albedoTex;
};

out vec3 fragColour;

in vec3 normal;
in vec2 uv;
in vec3 fragPos;

uniform Material material;

uniform sampler2D Albedo;

uniform vec3 lightPos;

void main() {
    vec3 dirToLight = normalize(lightPos - fragPos);
    float diff = max(dot(normal, dirToLight), 0.03);
    // fragColour = vec3(normal * 0.5 + 0.5) * diff;
    // fragColour = texture(Albedo, uv).rgb * diff;
    vec3 texCol = vec3(1);
    if (material.hasTexture == 1) texCol = texture(material.albedoTex, uv).rgb;
    vec3 diffuse = material.diffuse * texCol * diff;
    vec3 emission = material.emission.rgb * material.emission.a;

    if (material.emissive == 1) {
        fragColour = emission;
    } else {
        fragColour = diffuse + emission;
    }
}
