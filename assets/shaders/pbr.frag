#version 460
#extension GL_KHR_vulkan_glsl: enable

layout (location = 0) in vec3 worldPosition;
layout (location = 1) in vec2 uv;
layout (location = 2) in mat3 tbn;

layout (location = 0) out vec4 fragColor;

layout (push_constant) uniform constants {
    vec4 camPos;
} PushConstants;

layout (set=0, binding=1) uniform samplerCube irradianceMap;
layout (set=0, binding=2) uniform samplerCube prefilterMap;
layout (set=0, binding=3) uniform sampler2D brdfLUT;

layout (set=2, binding=0) uniform sampler2D albedoTex;
layout (set=2, binding=1) uniform sampler2D materialTex;
layout (set=2, binding=2) uniform sampler2D normalTex;

const float PI = 3.14159265359;

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}  

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}   

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

vec3 uncharted2_tonemap_partial(vec3 x)
{
    float A = 0.15f;
    float B = 0.50f;
    float C = 0.10f;
    float D = 0.20f;
    float E = 0.02f;
    float F = 0.30f;
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 uncharted2_filmic(vec3 v)
{
    float exposure_bias = 2.0f;
    vec3 curr = uncharted2_tonemap_partial(v * exposure_bias);

    vec3 W = vec3(11.2f);
    vec3 white_scale = vec3(1.0f) / uncharted2_tonemap_partial(W);
    return curr * white_scale;
}

vec3 aces_approx(vec3 v)
{
    v *= 0.6f;
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((v*(a*v+b))/(v*(c*v+d)+e), 0.0f, 1.0f);
}

void main() {
    vec3 albedo = texture(albedoTex, uv).rgb;
    float metallic = clamp(texture(materialTex, uv).r, 0.05, 0.95);
    float roughness = clamp(texture(materialTex, uv).g, 0.05, 0.95);
    float ao = 0.98f;
    
    vec3 f0 = vec3(0.04f);
    f0 = mix(f0, albedo, metallic);

    vec3 n = texture(normalTex, uv).rgb;
    n = n * 2.0 - 1.0;
    n = normalize(tbn * n);

    vec3 v = normalize(PushConstants.camPos.xyz - worldPosition);
    vec3 r = reflect(-v, n);

    vec3 Lo = vec3(0.0f);
    for (int i=0; i < 1; i++) { // Just the one light
        vec3 lightPosition = vec3(10.0f, 10.0f, 10.0f);
        vec3 l = normalize(lightPosition - worldPosition);
        vec3 h = normalize(v + l);

        float distance = length(lightPosition - worldPosition);
        float attenuation = 1.0f / (distance * distance);
        vec3 radiance = vec3(300.0f, 300.0f, 300.0f) * attenuation;

        float ndf = DistributionGGX(n, h, roughness);
        float g = GeometrySmith(n, v, l, roughness);
        vec3 f = fresnelSchlick(max(dot(h, v), 0.0), f0);

        vec3 numerator = ndf * g * f;
        float denominator = 4.0f * max(dot(n,v), 0.0f) * max(dot(n, l), 0.0f);
        vec3 specular = numerator / max(denominator, 0.001);

        vec3 kS = f;
        vec3 kD = vec3(1.0f) - kS;
        kD *= 1.0f - metallic;

        float nDotL = max(dot(n, l), 0.0f);
        Lo += (kD * albedo / PI + specular) * radiance * nDotL;
    }

    vec3 f = fresnelSchlickRoughness(max(dot(n, v), 0.0), f0, roughness);

    vec3 kS = f;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;	  

    vec3 irradiance = texture(irradianceMap, n).rgb;
    vec3 diffuse      = irradiance * albedo;

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilterColor = textureLod(prefilterMap, r, roughness * MAX_REFLECTION_LOD).rgb;
    vec2 brdf = texture(brdfLUT, vec2(max(dot(n,v), 0.0), roughness)).rg;
    vec3 specular = prefilterColor * (f * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * ao;

    vec3 color = ambient + Lo;

    //color = color / (color + vec3(1.0)); // Default
    //color = uncharted2_filmic(color); // Uncharted
    color = aces_approx(color); // Aces Approximation

    color = pow(color, vec3(1.0 / 2.2));

    fragColor = vec4(color, 1.0f);
}