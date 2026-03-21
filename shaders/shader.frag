// shader.frag

#version 450

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in flat uint fragBlockType;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 viewPos;
} ubo;

layout(binding = 1) uniform sampler2D texSampler;  // ← 追加

layout(location = 0) out vec4 outColor;

float getBlockAlpha(uint blockType) {
    if (blockType == 5u) return 0.8;  // Leaves
    if (blockType == 7u) return 0.6;  // Water
    return 1.0;
}

void main() {
    vec4 texColor = texture(texSampler, fragTexCoord);  // ← テクスチャからサンプリング

    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    vec3 normal   = normalize(fragNormal);
    float ambient = 0.3;
    float diffuse = max(dot(normal, lightDir), 0.0);
    vec3 lighting = vec3(ambient + diffuse * 0.7);

    vec3 finalColor = texColor.rgb * lighting;
    finalColor = pow(finalColor, vec3(1.0 / 2.2));

    float alpha = texColor.a * getBlockAlpha(fragBlockType);
    outColor = vec4(finalColor, texColor.a);
}