#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outPosition;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    vec4 color;
    mat4 model;
    mat4 view;
    mat4 projection;
    ivec4 userInput;
    vec4 camera_pos;
} UBO;

void main() {
    outNormal = normalize(mat3(UBO.model) * inNormal);outNormal = normalize(mat3(UBO.model) * inNormal);
    //outNormal = normalize(transpose(inverse(mat3(UBO.model))) * inNormal);
    outColor = inColor;
    outPosition = (UBO.model * vec4(inPosition, 1.0)).xyz;
    gl_Position = UBO.projection * UBO.view * vec4(inPosition, 1.0);
}