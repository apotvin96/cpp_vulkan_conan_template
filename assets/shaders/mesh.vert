#version 460
#extension GL_KHR_vulkan_glsl: enable

layout (location=0) in vec3 position;
layout (location=1) in vec2 uv;
layout (location=2) in vec3 normal;

layout (set=0, binding=0) uniform CameraBuffer {
	mat4 view;
	mat4 proj;
	mat4 viewProj;
} cameraData;

struct ObjectData{
	mat4 model;
};

layout(std140, set = 1, binding = 0) readonly buffer ObjectBuffer{
	ObjectData objects[];
} objectBuffer;

layout (location=0) out vec2 outUV;

void main()
{
	outUV = uv;
	//output the position of each vertex
	gl_Position = cameraData.viewProj * objectBuffer.objects[gl_BaseInstance].model * vec4(position, 1.0f);
}
