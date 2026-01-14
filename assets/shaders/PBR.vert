#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outColor;
layout (location = 3) out flat int instanceID;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    vec4 color;
    mat4 model;
    mat4 view;
    mat4 projection;
    ivec4 userInput;
    vec4 camera_pos;
    vec4 material;
} UBO;

void main() {
    outNormal = normalize(mat3(transpose(inverse(UBO.model))) * inNormal);
    instanceID = gl_InstanceIndex;
    float x_offset = float(instanceID % 10) * 1.0f;
    float y_offset = float(instanceID / 10) * 1.0f;
    outColor = vec3(0.8f - (y_offset / 9.0f) * 0.3f);
    outPosition = vec3(UBO.model * vec4(inPosition + vec3(x_offset - 4.5f, y_offset - 4.5f, -2.0f), 1.0));
    gl_Position = UBO.projection * UBO.view * vec4(outPosition, 1.0f);
}
