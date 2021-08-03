#version 450 core

layout(binding = 0, rgba8) uniform image2D gScene;
layout(binding = 1, rg16ui) uniform uimage2D gCompressionTexture;

out vec4 outputColor;

layout(origin_upper_left) in vec4 gl_FragCoord;

uniform int gHeight;

void main()
{
	int x = int(gl_FragCoord.x);
	int y = int(gl_FragCoord.y + 0.5f);

	ivec2 texCoord = ivec2(x,y);

	//Get mapping coord
	uvec2 mapping = imageLoad(gCompressionTexture, texCoord).rg;

	//Flip
	texCoord = ivec2(mapping);
	texCoord.y = gHeight - texCoord.y -1;

	//Get color from that mapped coord
	outputColor = imageLoad(gScene, texCoord);
}