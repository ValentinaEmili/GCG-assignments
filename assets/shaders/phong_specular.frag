#version 450
layout(location = 0) in vec3 outPosition;
layout(location = 1) in vec3 outNormal;
layout(location = 2) in vec3 outColor;
layout(location = 3) in vec2 outTexCoords;

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

layout(set = 0, binding = 1) uniform DirectionalLightUBO {
    vec4 color;
    vec4 direction;
} dirLightUBO;

layout(set = 0, binding = 2) uniform PointLightUBO {
    vec4 color;
    vec4 position;
    vec4 attenuation;
} pointLightUBO;

layout(set = 0, binding = 3) uniform sampler2D diffuse_texture;
layout(set = 0, binding = 4) uniform sampler2D specular_texture;

void main() {
    float ka = UBO.material.x;
    float kd = UBO.material.y;
    float ks = UBO.material.z;
    float alpha = UBO.material.w;
    vec3 ambient = ka * vec3(1.0f);

    vec3 V = normalize(UBO.camera_pos.xyz - outPosition);
    vec3 N = normalize(outNormal);

    // directional light
    vec3 Ld = normalize(-dirLightUBO.direction.xyz);
    float diff_dir = max(dot(N, Ld), 0.0f);
    vec3 Rd = reflect(-Ld, N);
    float spec_dir = pow(max(dot(Rd, V), 0.0f), alpha);

    // point light
    vec3 Lp = normalize(pointLightUBO.position.xyz - outPosition);
    float dist = length(pointLightUBO.position.xyz - outPosition);
    float diff_point = max(dot(N, Lp), 0.0f);
    vec3 Rp = reflect(-Lp, N);
    float spec_point = pow(max(dot(Rp, V), 0.0f), alpha);

    float attenuation = 1.0f / (pointLightUBO.attenuation.x + pointLightUBO.attenuation.y * dist + pointLightUBO.attenuation.z * dist * dist);
    vec3 diffuse = kd * (diff_dir * dirLightUBO.color.rgb + diff_point * pointLightUBO.color.rgb * attenuation);
    vec3 specular = ks * (spec_dir * dirLightUBO.color.rgb + spec_point * pointLightUBO.color.rgb * attenuation);

    vec3 texture_color = texture(diffuse_texture, outTexCoords).rgb;
    vec3 specular_texture_color = texture(specular_texture, outTexCoords).rgb;
    vec3 result_color = (ambient + diffuse) * texture_color;

    if (UBO.userInput[0] == 1) {
        vec3 scaledNormal = 0.5f * N + 0.5f;
        fragColor = vec4(pow(scaledNormal, vec3(2.2)), 1.0f);
        return;
    }
    else if (UBO.userInput[2] == 1) {
        fragColor = vec4(outTexCoords, 0.0f, 1.0f);
    }
    else if (UBO.userInput[1] == 1) {
        float F0 = 0.1f;
        float cos_theta = max(dot(N, V), 0.0f);
        float fresnel_coeff = F0 + (1.0f - F0) * pow(1.0f - cos_theta, 5.0f);
        vec3 color = result_color * (1.0f - fresnel_coeff) + specular * specular_texture_color * fresnel_coeff;
        fragColor = vec4(color, 1.0f);
    }
    else {
        fragColor = vec4(result_color + specular * specular_texture_color, 1.0f);
    }
}
