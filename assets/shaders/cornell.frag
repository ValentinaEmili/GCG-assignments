#version 450
#define MAX_N_LIGHTS 100

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

layout(set = 0, binding = 1) uniform DirectionalLightUBOs {
    DirectionalLightUBO lights[MAX_N_LIGHTS];
    int num_lights;
    vec3 _pad;
} dirLightUBOs;
layout(set = 0, binding = 2) uniform PointLightUBOs {
    PointLightUBO lights[MAX_N_LIGHTS];
    int num_lights;
    vec3 _pad;
} pointLightUBOs;
layout(set = 0, binding = 3) uniform SpotLightUBOs {
    SpotLightUBO lights[MAX_N_LIGHTS];
    int num_lights;
    vec3 _pad;
} spotLightUBOs;


void main() {
    vec3 V = normalize(UBO.camera_pos.xyz - outPosition);
    vec3 N = normalize(outNormal);
    float facing = dot(V, N);
    if (facing < 0.0f) {
        N = -N;
    }
    // normals view
    if (UBO.userInput[0] == 1) {
        vec3 scaledNormal = 0.5f * N + 0.5f;
        fragColor = vec4(pow(scaledNormal, vec3(2.2)), 1.0f);
        return;
    }
    vec3 output_color = (UBO.userInput[2] == 1) ? outColor : UBO.color.rgb;
    if (facing > 0.0f) {
        fragColor = vec4(output_color, 1.0f);
    }
    else {
        float ka = UBO.material.x;
        float kd = UBO.material.y;
        float ks = UBO.material.z;
        float alpha = UBO.material.w;
        // ambient light
        vec3 ambient = ka * vec3(1.0f);
        // directional light
        vec3 diff_dir = vec3(0.0f);
        vec3 spec_dir = vec3(0.0f);
        for (int i = 0; i < dirLightUBOs.num_lights; i++) {

            vec3 Ld = normalize(-dirLightUBOs.lights[i].direction.xyz);
            float diff = max(dot(N, Ld), 0.0f);
            diff_dir += diff * dirLightUBOs.lights[i].color.rgb;

            vec3 Rd = reflect(-Ld, N);
            float spec = pow(max(dot(Rd, V), 0.0f), alpha);
            spec_dir += spec * dirLightUBOs.lights[i].color.rgb;
        }
        // point light
        vec3 diff_point = vec3(0.0f);
        vec3 spec_point = vec3(0.0f);
        for (int i = 0; i < pointLightUBOs.num_lights; i++) {

            float dist = length(pointLightUBOs.lights[i].position.xyz - outPosition);
            float attenuation = 1.0f / (pointLightUBOs.lights[i].attenuation.x + pointLightUBOs.lights[i].attenuation.y * dist + pointLightUBOs.lights[i].attenuation.z * dist * dist);

            vec3 Lp = normalize(pointLightUBOs.lights[i].position.xyz - outPosition);
            float diff = max(dot(N, Lp), 0.0f);
            diff_point += diff * pointLightUBOs.lights[i].color.rgb * attenuation;

            vec3 Rp = reflect(-Lp, N);
            float spec = pow(max(dot(Rp, V), 0.0f), alpha);
            spec_point += spec * pointLightUBOs.lights[i].color.rgb * attenuation;
        }
        // spot light
        vec3 diff_spot = vec3(0.0f);
        vec3 spec_spot = vec3(0.0f);
        for (int i = 0; i < spotLightUBOs.num_lights; i++) {
            float dist = length(spotLightUBOs.lights[i].position.xyz - outPosition);
            float attenuation = 1.0f / (spotLightUBOs.lights[i].attenuation.x + spotLightUBOs.lights[i].attenuation.y * dist + spotLightUBOs.lights[i].attenuation.z * dist * dist);

            vec3 Ls = normalize(spotLightUBOs.lights[i].position.xyz - outPosition);
            vec3 spot_dir = normalize(spotLightUBOs.lights[i].direction.xyz);
            float spot_effect = smoothstep(spotLightUBOs.lights[i].outer_radius.x, spotLightUBOs.lights[i].inner_radius.x, tan(acos(dot(-Ls, spot_dir))));

            float diff = max(dot(N, Ls), 0.0f);
            diff_spot += diff * spotLightUBOs.lights[i].color.rgb * attenuation * spot_effect;

            vec3 Rs = reflect(-Ls, N);
            float spec = pow(max(dot(Rs, V), 0.0f), alpha);
            spec_spot += spec * spotLightUBOs.lights[i].color.rgb * attenuation * spot_effect;
        }

        vec3 diffuse = kd * (diff_dir + diff_point + diff_spot);
        vec3 specular = ks * (spec_dir + spec_point + spec_spot);
        fragColor = vec4((ambient + diffuse) * output_color + specular, 1.0f);
    }
}
