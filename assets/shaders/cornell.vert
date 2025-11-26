#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor; // to assign a different color for each face (Cornell Box)
layout(location = 0) out vec3 fragColor;
layout(set = 0, binding = 0) uniform UniformBufferObject {
    vec4 color;
    mat4 view_projection;
} ubo;

void main() {
    fragColor = inColor;
    gl_Position = ubo.view_projection * vec4(inPosition, 1.0);
}
