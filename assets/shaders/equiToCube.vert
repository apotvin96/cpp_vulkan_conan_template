#version 460
#extension GL_KHR_vulkan_glsl: enable

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

layout (location = 0) out vec3 localPos;

layout (push_constant) uniform CameraBuffer {
	mat4 viewProj;
} cameraData;

void main() {
    localPos = position;
    gl_Position = cameraData.viewProj * vec4(localPos, 1.0);
}