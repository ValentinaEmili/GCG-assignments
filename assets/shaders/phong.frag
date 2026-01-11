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

layout(location = 0) in vec3 outPosition;
layout(location = 1) in vec3 outNormal;
layout(location = 2) in vec3 outColor;

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

vec3 getCornellBoxReflectionColor(vec3 positionWS, vec3 directionWS) {
    vec3 P0 = positionWS;
    vec3 V  = normalize(directionWS);
    const float boxSize = 1.5;
    vec4[5] planes = {
    vec4(-1.0,  0.0,  0.0, -boxSize), // left
    vec4( 1.0,  0.0,  0.0, -boxSize), // right
    vec4( 0.0,  1.0,  0.0, -boxSize), // top
    vec4( 0.0, -1.0,  0.0, -boxSize), // bottom
    vec4( 0.0,  0.0, -1.0, -boxSize)  // back
    };
    vec3[5] colors = {
    vec3(0.49, 0.06, 0.22),    // left
    vec3(0.0, 0.13, 0.31),    // right
    vec3(0.96, 0.93, 0.85), // top
    vec3(0.64, 0.64, 0.64), // bottom
    vec3(0.76, 0.74, 0.68)  // back
    };
    for (int i = 0; i < 5; ++i) {
        vec3  N = planes[i].xyz;
        float d = planes[i].w;
        float denom = dot(V, N);
        if (denom <= 0) continue;
        float t = -(dot(P0, N) + d)/denom;
        vec3  P = P0 + t*V;
        float q = boxSize + 0.01;
        if (P.x > -q && P.x < q && P.y > -q && P.y < q && P.z > -q && P.z < q) {
            return colors[i];
        }
    }
    return vec3(0.0, 0.0, 0.0);
}
vec3 clampedReflect(vec3 I, vec3 N) {
    return I - 2.0 * min(dot(N, I), 0.0) * N;
}

void main() {
    float ka = UBO.material.x;
    float kd = UBO.material.y;
    float ks = UBO.material.z;
    float alpha = UBO.material.w;
    vec3 ambient = ka * vec3(1.0f);

    vec3 V = normalize(UBO.camera_pos.xyz - outPosition);
    vec3 N = normalize(outNormal);

    if (UBO.userInput[0] == 1) {
        vec3 scaledNormal = 0.5f * N + 0.5f;
        fragColor = vec4(pow(scaledNormal, vec3(2.2)), 1.0f);
        return;
    }
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
        float spot_effect = smoothstep(spotLightUBOs.lights[i].outer_radius.x, spotLightUBOs.lights[i].inner_radius.x, dot(Ls, spot_dir));

        float diff = max(dot(N, Ls), 0.0f);
        diff_spot += diff * spotLightUBOs.lights[i].color.rgb * attenuation * spot_effect;

        vec3 Rs = reflect(-Ls, N);
        float spec = pow(max(dot(Rs, V), 0.0f), alpha);
        spec_spot += spec * spotLightUBOs.lights[i].color.rgb * attenuation * spot_effect;
    }

    vec3 diffuse = kd * (diff_dir + diff_point + diff_spot);
    vec3 specular = ks * (spec_dir + spec_point + spec_spot);
    vec3 result_color = (ambient + diffuse) * outColor + specular;

    if (UBO.userInput[1] == 1) {
        float F0 = 0.1f;
        float cos_theta = max(dot(N, V), 0.0f);
        float fresnel_coeff = F0 + (1.0f - F0) * pow(1.0f - cos_theta, 5.0f);

        vec3 outDirection = clampedReflect(-V, N);
        vec3 reflection_color = getCornellBoxReflectionColor(outPosition, outDirection);
        vec3 color = mix(result_color, reflection_color, fresnel_coeff);
        fragColor = vec4(color, 1.0f);
    }
    else {
        fragColor = vec4(result_color, 1.0f);
    }
}
