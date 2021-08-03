#version 430 core

uniform sampler2D mytexture;

noperspective in vec2 texCoord;

out vec4 outputColor;

void main()
{
	outputColor = texture(mytexture, texCoord);
}