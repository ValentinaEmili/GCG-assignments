#version 450
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

layout(set = 0, binding = 1) uniform DirectionalLightUBO {
    vec4 color;
    vec4 direction;
} dirLightUBO;

layout(set = 0, binding = 2) uniform PointLightUBO {
    vec4 color;
    vec4 position;
    vec4 attenuation;
} pointLightUBO;

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
    vec3 N = normalize(outNormal);
    vec3 V = normalize(UBO.camera_pos.xyz - outPosition);
    float facing = dot(V, N);

    if (facing < 0.0f) {
        N = -N;
    }

    if (UBO.userInput.x == 1) {
        vec3 scaledNormal = 0.5f * N + 0.5f;
        fragColor = vec4(pow(scaledNormal[0], 2.2), pow(scaledNormal[1], 2.2), pow(scaledNormal[2], 2.2), 1.0f);
        return;
    }

    vec3 objectColor = (UBO.userInput.z == 1) ? outColor : UBO.color.rgb;

    float ka = UBO.material.x;
    float kd = UBO.material.y;
    float ks = UBO.material.z;
    float alpha = UBO.material.w;

    vec3 ambientLight = ka * vec3(1.0f);

    // directional light
    vec3 L = normalize(-dirLightUBO.direction.xyz);
    vec3 R = 2.0f * max(dot(L, N), 0.0f) * N - L;
    vec3 specularLight = ks * pow(max(dot(R, V), 0.0f), alpha) * dirLightUBO.color.rgb;
    vec3 diffuseLight = kd * max(dot(L, N), 0.0f) * dirLightUBO.color.rgb;
    vec3 directional_illumination = diffuseLight + specularLight;

    // point light
    float dist = length(pointLightUBO.position.xyz - outPosition);
    L = normalize(pointLightUBO.position.xyz - outPosition);
    R = 2.0f * max(dot(L, N), 0.0f) * N - L;
    specularLight = ks * pow(max(dot(R, V), 0.0f), alpha) * pointLightUBO.color.rgb;
    diffuseLight = kd * max(dot(L, N), 0.0f) * pointLightUBO.color.rgb;
    float attenuation = 1.0f / (pointLightUBO.attenuation.x + pointLightUBO.attenuation.y * dist + pointLightUBO.attenuation.z * dist * dist);
    vec3 point_light_illumination = attenuation * (diffuseLight + specularLight);

    vec3 global_illumination = ambientLight + directional_illumination + point_light_illumination;

    if (UBO.userInput.y == 1) {
        float F0 = 0.1f;
        float cos_theta = max(dot(normalize(N), V), 0.0f);
        float fresnel_coeff = F0 + (1.0f - F0) * pow(1.0f - cos_theta, 5.0f);

        vec3 outDirection = clampedReflect(-V, N);
        vec3 reflection_color = getCornellBoxReflectionColor(outPosition, outDirection);

        vec3 color = mix(objectColor * global_illumination, reflection_color, fresnel_coeff);
        //vec3 color = output_color.rgb * global_illumination + fresnel_coeff * reflection_color;
        fragColor = vec4(color, 1.0f);
    }
    else {
        fragColor = vec4(global_illumination * objectColor, 1.0f);
    }
}
