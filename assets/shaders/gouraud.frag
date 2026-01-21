#version 450
layout(location = 0) in vec3 outNormal;
layout(location = 1) in vec3 outColor;
layout(location = 2) in vec2 outTexCoords;

layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    vec4 color;
    mat4 model;
    mat4 view;
    mat4 projection;
    ivec4 userInput;
    vec4 camera_pos;
    vec4 material;
    vec4 texture;
} UBO;

void main() {
    // normals
    if (UBO.userInput[0] == 1) {
        vec3 scaledNormal = 0.5f * outNormal + 0.5f;
        fragColor = vec4(pow(scaledNormal, vec3(2.2)), 1.0f);
    }
    // texture coords
    else if (UBO.userInput[2] == 1) {
        fragColor = vec4(outTexCoords, 0.0f, 1.0f);
    }
    else {
        fragColor = vec4(outColor, 1.0f);
    }
}
