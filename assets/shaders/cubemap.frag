#version 460
#extension GL_KHR_vulkan_glsl: enable

layout (location = 0) in vec3 localPos;

layout (location = 0) out vec4 FragColor;

layout (set=1, binding = 0) uniform samplerCube environmentMap;

layout (push_constant) uniform CameraBuffer {
    mat4 view;
    mat4 projection;
	mat4 viewProj;
} cameraData;


void main() {
    vec3 envColor = textureLod(environmentMap, localPos, 0.0).rgb;

	FragColor = vec4(envColor, 1.0);
}
