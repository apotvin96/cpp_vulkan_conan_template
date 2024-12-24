//glsl version 4.5
#version 450

//output write
layout (location = 0) out vec4 outFragColor;

layout( push_constant ) uniform constants
{
	vec4 posOffset;
	vec4 colorOffset;
} PushConstants;

void main()
{
	outFragColor = vec4(0.0f + PushConstants.colorOffset.r,0.0f + PushConstants.colorOffset.g,0.0f,1.0f);
}
