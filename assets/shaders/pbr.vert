#version 460
#extension GL_KHR_vulkan_glsl: enable

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in vec2 uv;

layout (location = 0) out vec3 outWorldPosition;
layout (location = 1) out vec2 outUV;
layout (location = 2) out mat3 tbn;

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

layout (push_constant) uniform constants {
    vec4 camPos;
} PushConstants;

void main() {
    outUV = uv;
	outWorldPosition = vec3(objectBuffer.objects[gl_BaseInstance].model * vec4(position, 1.0f));

	vec3 t = normalize(vec3(objectBuffer.objects[gl_BaseInstance].model * vec4(tangent, 0.0f)));
	vec3 n = normalize(vec3(objectBuffer.objects[gl_BaseInstance].model * vec4(normal, 0.0f)));
	t = normalize(t - dot(t, n) * n);
	vec3 b = cross(n, t) * -1;
	tbn = mat3(t, b, n);

	gl_Position = cameraData.viewProj * vec4(outWorldPosition, 1.0f);
}