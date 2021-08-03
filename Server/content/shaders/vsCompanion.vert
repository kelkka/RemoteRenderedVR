#version 430 core

layout(location = 0) in vec4 in_position;
layout(location = 1) in vec2 in_texCoord;

noperspective out vec2 texCoord;

void main()
{
	texCoord = in_texCoord;
	texCoord.y = 1 - in_texCoord.y;
	gl_Position = in_position;
}
