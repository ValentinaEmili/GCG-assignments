#version 450
layout(location = 0) in vec3 outColor;
layout(location = 1) in vec3 outNormal;
layout(location = 2) in vec3 outPosition;

layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    vec4 color;
    mat4 model;
    mat4 view;
    mat4 projection;
    ivec4 userInput;
    vec4 camera_pos;
    vec4 material;
} UBO;

layout(set = 0, binding = 1) uniform DirectionalLightUBO {
    vec4 color;
    vec4 direction;
} dirLightUBO;

layout(set = 0, binding = 2) uniform PointLightUBO {
    vec4 color;
    vec4 position;
    vec4 attenuation;
} pointLightUBO;

void main() {
    vec3 V = normalize(UBO.camera_pos.xyz - outPosition);
    vec3 N = normalize(outNormal);
    float facing = dot(V, N);

    // normals view
    if (UBO.userInput[0] == 1) {
        if (facing < 0.0f) {
            N = -N;
        }
        vec3 scaledNormal = 0.5f * N + 0.5f;
        fragColor = vec4(pow(scaledNormal[0], 2.2), pow(scaledNormal[1], 2.2), pow(scaledNormal[2], 2.2), 1.0f);
        return;
    }

    if (facing > 0.0f) {
        fragColor = vec4(outColor, 1.0f);
    }
    else {
        vec3 output_color = (UBO.userInput[2] == 1) ? outColor : UBO.color.rgb;
        float ka = UBO.material.x;
        float kd = UBO.material.y;
        float ks = UBO.material.z;
        float alpha = UBO.material.w;

        vec3 worldPosition = vec3(UBO.model * vec4(outPosition, 1.0));


        vec3 ambientLight = ka * vec3(1.0f);

        // directional light
        vec3 L = normalize(-dirLightUBO.direction.xyz);
        vec3 R = 2.0f * max(dot(L, N), 0.0f) * N - L;
        vec3 specularLight = ks * pow(max(dot(R, V), 0.0f), alpha) * dirLightUBO.color.rgb;
        vec3 diffuseLight = kd * max(dot(L, N), 0.0f) * dirLightUBO.color.rgb;
        vec3 directional_illumination = diffuseLight + specularLight;

        // point light
        float dist = length(pointLightUBO.position.xyz - worldPosition);
        L = normalize(pointLightUBO.position.xyz - worldPosition);
        R = 2.0f * max(dot(L, N), 0.0f) * N - L;
        specularLight = ks * pow(max(dot(R, V), 0.0f), alpha) * pointLightUBO.color.rgb;
        diffuseLight = kd * max(dot(L, N), 0.0f) * pointLightUBO.color.rgb;
        float attenuation = 1.0f / (pointLightUBO.attenuation.x + pointLightUBO.attenuation.y * dist + pointLightUBO.attenuation.z * dist * dist);
        vec3 point_light_illumination = attenuation * (diffuseLight + specularLight);

        vec3 global_illumination = ambientLight + directional_illumination + point_light_illumination;

        fragColor = vec4(global_illumination * output_color, 1.0f);

    }
}
