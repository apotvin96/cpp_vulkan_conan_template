//we will be using glsl version 4.5 syntax
#version 450
#extension GL_KHR_vulkan_glsl: enable

layout( push_constant ) uniform constants
{
	vec4 posOffset;
	vec4 colorOffset;
} PushConstants;

void main()
{
	//const array of positions for the triangle
	const vec3 positions[3] = vec3[3](
		vec3(1.0f + PushConstants.posOffset.x,1.0f, 0.0f),
		vec3(-1.0f + PushConstants.posOffset.x,1.0f, 0.0f),
		vec3(0.0f + PushConstants.posOffset.x,-1.0f, 0.0f)
	);

	//output the position of each vertex
	gl_Position = vec4(positions[gl_VertexIndex], 1.0f);
}
