#version 450
layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_texCoord;

void main()
{
	gl_Position = vec4(in_position, 0, 1);
}