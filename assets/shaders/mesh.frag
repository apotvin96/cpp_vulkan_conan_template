#version 460
#extension GL_KHR_vulkan_glsl: enable

layout (location = 0) in vec2 uv;

layout (location = 0) out vec4 outFragColor;

layout (set=2, binding=0) uniform sampler2D colorTex;

void main()
{
	vec3 texColor = texture(colorTex, uv).rgb;
	outFragColor = vec4(texColor.r, texColor.g, texColor.b,1.0f);
}
