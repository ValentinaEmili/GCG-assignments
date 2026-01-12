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


void main() {
    vec3 V = normalize(UBO.camera_pos.xyz - outPosition);
    vec3 N = normalize(outNormal);
    float facing = dot(V, N);
    if (facing < 0.0f) {
        N = -N;
    }
    else {

    }
    // normals view
    if (UBO.userInput[0] == 1) {
        vec3 scaledNormal = 0.5f * N + 0.5f;
        fragColor = vec4(pow(scaledNormal, vec3(2.2)), 1.0f);
        return;
    }
    float ka = UBO.material.x;
    float kd = UBO.material.y;
    float ks = UBO.material.z;
    float alpha = UBO.material.w;
    vec3 ambient = ka * vec3(1.0f);
    vec3 output_color = (UBO.userInput[2] == 1) ? outColor : UBO.color.rgb;

    /// directional light
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

    if (facing > 0.0f) {
        fragColor = vec4(output_color, 1.0f);
    }
    else {
        fragColor = vec4((ambient + diffuse) * output_color + specular, 1.0f);
    }
}
