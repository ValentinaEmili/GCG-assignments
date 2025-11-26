#version 450
layout(location = 0) out vec4 outColor;
layout(location = 0) in vec3 fragColor;
layout(set = 0, binding = 0) uniform UniformBufferObject {
    vec4 color;
    mat4 view_projection;
} ubo;

void main() {
    outColor = vec4(fragColor, 1.0f);

}
