#version 460
#extension GL_KHR_vulkan_glsl: enable

layout (location = 0) in vec3 localPos;

layout (location = 0) out vec4 FragColor;

layout (set=0, binding = 0) uniform sampler2D equirectangularMap;

layout (push_constant) uniform CameraBuffer {
	mat4 viewProj;
} cameraData;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main() {		
    vec2 uv = SampleSphericalMap(normalize(localPos)); // make sure to normalize localPos
    vec3 color = texture(equirectangularMap, uv).rgb;
    
    FragColor = vec4(color, 1.0);
}
