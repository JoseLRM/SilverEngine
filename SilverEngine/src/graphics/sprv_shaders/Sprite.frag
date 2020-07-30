#version 450

layout (location = 0) in vec4 FragColor;
layout (location = 1) in vec2 FragTexCoord;

layout (location = 0) out vec4 outColor;

layout (binding = 0) uniform sampler sam;
layout (binding = 1) uniform texture2D tex;

void main()
{
	outColor = FragColor * texture(sampler2D(tex, sam), FragTexCoord);
}