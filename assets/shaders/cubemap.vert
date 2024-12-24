#version 460
#extension GL_KHR_vulkan_glsl: enable

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

layout (location = 0) out vec3 localPos;

layout (set=0, binding=0) uniform CameraBuffer {
    mat4 view;
    mat4 projection;
	mat4 viewProj;
} cameraData;

void main() {
    localPos = position;

    mat4 rotView = mat4(mat3(cameraData.view)); // remove translation from the view matrix
    vec4 clipPos = cameraData.projection * rotView * vec4(localPos, 1.0);

    gl_Position = clipPos.xyww;
}