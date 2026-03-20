#version 450

// 入力
layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in flat uint fragBlockType;

// ユニフォームバッファ
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 viewPos;
} ubo;

// 出力
layout(location = 0) out vec4 outColor;

// ブロックタイプごとの色を返す
vec3 getBlockColor(uint blockType) {
    switch(blockType) {
        case 1: return vec3(0.4, 0.8, 0.2);  // Grass - 緑
        case 2: return vec3(0.6, 0.4, 0.2);  // Dirt - 茶色
        case 3: return vec3(0.5, 0.5, 0.5);  // Stone - 灰色
        case 4: return vec3(0.6, 0.3, 0.1);  // Wood - 木の色
        case 5: return vec3(0.2, 0.6, 0.2);  // Leaves - 濃い緑
        case 6: return vec3(0.9, 0.8, 0.6);  // Sand - 砂色
        case 7: return vec3(0.2, 0.4, 0.8);  // Water - 青
        default: return vec3(1.0, 1.0, 1.0); // Air/Unknown - 白
    }
}

// ★ 追加：ブロックタイプごとのアルファ値を返す
float getBlockAlpha(uint blockType) {
    switch(blockType) {
        case 5: return 0.8;  // Leaves - 少し透明
        case 7: return 0.6;  // Water - 透明
        default: return 1.0; // その他 - 不透明
    }
}

void main() {
    vec3 baseColor = getBlockColor(fragBlockType);
    float alpha = getBlockAlpha(fragBlockType);  // ★ 追加
    
    // 簡易ライティング
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));  // 太陽の方向
    vec3 normal = normalize(fragNormal);
    
    // アンビエント
    float ambient = 0.3;
    
    // ディフューズ
    float diffuse = max(dot(normal, lightDir), 0.0);
    
    // 最終的な色
    vec3 lighting = vec3(ambient + diffuse * 0.7);
    vec3 finalColor = baseColor * lighting;
    
    // ガンマ補正
    finalColor = pow(finalColor, vec3(1.0/2.2));
    
    outColor = vec4(finalColor, alpha);  // ★ 変更：アルファ値を使用
}