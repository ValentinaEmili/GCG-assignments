#version 450
#define MAX_N_LIGHTS 100
#define M_PI 3.1415926535897932384626433832795
#define IOR 0.04

struct DirectionalLightUBO {
    vec4 color;
    vec4 direction;
};
struct PointLightUBO {
    vec4 color;
    vec4 position;
    vec4 attenuation;
};
struct SpotLightUBO {
    vec4 color;
    vec4 position;
    vec4 outer_radius;
    vec4 inner_radius;
    vec4 direction;
    vec4 attenuation;
};

layout(location = 0) in vec3 outPosition;
layout(location = 1) in vec3 outNormal;
layout(location = 2) in vec3 outColor;
layout (location = 3) in flat int instanceID;

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
layout(set = 0, binding = 1) uniform DirectionalLightUBOs {
    DirectionalLightUBO lights[MAX_N_LIGHTS];
    uint num_lights;
    vec3 _pad;
} dirLightUBOs;
layout(set = 0, binding = 2) uniform PointLightUBOs {
    PointLightUBO lights[MAX_N_LIGHTS];
    uint num_lights;
    vec3 _pad;
} pointLightUBOs;
layout(set = 0, binding = 3) uniform SpotLightUBOs {
    SpotLightUBO lights[MAX_N_LIGHTS];
    uint num_lights;
    vec3 _pad;
} spotLightUBOs;

float NormalDistributionFunctionGGXTR(vec3 N, vec3 H, float roughness) {
    float alpha2 = pow(roughness, 2);
    float NdotH2 = pow(max(dot(N,H), 0.0f), 2);
    float denom = NdotH2 * (alpha2 - 1) + 1;
    return alpha2 / (M_PI * pow(denom, 2));
}

float GeometrySchlickGGX(float NdotV, float k) {
    float denom = NdotV * (1.0f - k) + k;
    return NdotV / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float k = float(pow(roughness + 1, 2) / 8);
    float ggx1 = GeometrySchlickGGX(NdotV, k);
    float ggx2 = GeometrySchlickGGX(NdotL, k);
    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cos_theta, vec3 F0) {
    return F0 + (1.0f - F0) * pow(clamp(1.0f - cos_theta, 0.0f, 1.0f), 5.0f);
}

void main() {
    float ka = UBO.material.x;
    float kd = UBO.material.y;
    float ks = UBO.material.z;
    float alpha = UBO.material.w;

    vec3 ambient = ka * vec3(1.0f);

    vec3 V = normalize(UBO.camera_pos.xyz - outPosition);
    vec3 N = normalize(outNormal);

    float roughness = clamp(float(instanceID % 10) / 9.0, 0.05f, 1.0f);
    float metallic = clamp(float(instanceID / 10) / 9.0, 0.0f, 1.0f);

    vec3 F0 = vec3(IOR);
    F0 = mix(F0, outColor, metallic);
    vec3 L_out = vec3(0.0f);

    // directional light
    for (int i = 0; i < dirLightUBOs.num_lights; i++) {
        vec3 L = normalize(-dirLightUBOs.lights[i].direction.xyz);
        vec3 H = normalize(L + V);
        float D = NormalDistributionFunctionGGXTR(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = FresnelSchlick(max(dot(H, N), 0.0f), F0);
        float NdotV = max(dot(N, V) , 0.0001f);
        float NdotL = max(dot(N, L) , 0.0001f);
        vec3 specular = (D * F * G) / max(4.0f * NdotV * NdotL, 0.0001);
        vec3 kS = F;
        vec3 kD = vec3(1.0f) - kS;
        kD *= (1.0f - metallic);
        vec3 diffuse = kD * outColor / M_PI;
        L_out += (diffuse + specular) * dirLightUBOs.lights[i].color.rgb * NdotL;
    }
    // point light
    for (int i = 0; i < pointLightUBOs.num_lights; i++) {
        float dist = length(pointLightUBOs.lights[i].position.xyz - outPosition);
        float attenuation = 1.0f / (pointLightUBOs.lights[i].attenuation.x + pointLightUBOs.lights[i].attenuation.y * dist + pointLightUBOs.lights[i].attenuation.z * dist * dist);

        vec3 L = normalize(pointLightUBOs.lights[i].position.xyz - outPosition);
        vec3 H = normalize(L + V);
        float D = NormalDistributionFunctionGGXTR(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = FresnelSchlick(max(dot(H, N), 0.0f), F0);
        float NdotV = max(dot(N, V) , 0.0001f);
        float NdotL = max(dot(N, L) , 0.0001f);
        vec3 specular = (D * F * G) / max(4.0f * NdotV * NdotL, 0.0001);
        vec3 kS = F;
        vec3 kD = vec3(1.0f) - kS;
        kD *= (1.0f - metallic);
        vec3 diffuse = kD * outColor / M_PI;
        L_out += (diffuse + specular) * pointLightUBOs.lights[i].color.rgb * attenuation * NdotL;
    }
    // spot light
    for (int i = 0; i < spotLightUBOs.num_lights; i++) {
        float dist = length(spotLightUBOs.lights[i].position.xyz - outPosition);
        float attenuation = 1.0f / (spotLightUBOs.lights[i].attenuation.x + spotLightUBOs.lights[i].attenuation.y * dist + spotLightUBOs.lights[i].attenuation.z * dist * dist);

        vec3 Ls = normalize(spotLightUBOs.lights[i].position.xyz - outPosition);
        vec3 spot_dir = normalize(spotLightUBOs.lights[i].direction.xyz);
        float cos_theta = dot(-Ls, spot_dir);
        float spot_effect = 1.0f - smoothstep(spotLightUBOs.lights[i].inner_radius.x, spotLightUBOs.lights[i].outer_radius.x, tan(acos(cos_theta)));

        vec3 L = normalize(spotLightUBOs.lights[i].position.xyz - outPosition);
        vec3 H = normalize(L + V);
        float D = NormalDistributionFunctionGGXTR(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = FresnelSchlick(max(dot(H, N), 0.0f), F0);
        float NdotV = max(dot(N, V) , 0.0001f);
        float NdotL = max(dot(N, L) , 0.0001f);
        vec3 specular = (D * F * G) / max(4.0f * NdotV * NdotL, 0.0001);
        vec3 kS = F;
        vec3 kD = vec3(1.0f) - kS;
        kD *= (1.0f - metallic);
        vec3 diffuse = kD * outColor / M_PI;
        L_out += (diffuse + specular) * spotLightUBOs.lights[i].color.rgb * attenuation * spot_effect * NdotL;
    }

    vec3 result_color = ambient * outColor + L_out;
    fragColor = vec4(result_color, 1.0f);
}
