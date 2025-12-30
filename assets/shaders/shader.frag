#version 450
layout(location = 0) out vec4 fragColor;
layout(location = 1) in vec3 outNormal;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    vec4 color;
    mat4 model;
    mat4 view;
    mat4 projection;
    ivec4 userInput;
    vec4 camera_pos;
} UBO;

void main() {
    if (UBO.userInput[0] == 1) {
        vec3 scaledNormal = 0.5f * outNormal + 0.5f;
        fragColor = vec4(pow(scaledNormal[0], 2.2), pow(scaledNormal[1], 2.2), pow(scaledNormal[2], 2.2), 1.0f);
    }
    else {
        fragColor = UBO.color;
    }
}
