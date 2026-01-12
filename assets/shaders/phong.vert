#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoords;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outColor;
layout(location = 3) out vec2 outTexCoords;

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
    outTexCoords = inTexCoords;
    outColor = (UBO.userInput[2] == 1) ? inColor : UBO.color.rgb;;
    outNormal = normalize(mat3(transpose(inverse(UBO.model))) * inNormal);
    outPosition = vec3(UBO.model * vec4(inPosition, 1.0));
    gl_Position = UBO.projection * UBO.view * UBO.model * vec4(inPosition, 1.0);
}
