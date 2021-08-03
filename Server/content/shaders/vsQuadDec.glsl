#version 450
layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_texCoord;

out vec2 v2UVcoords;

void main()
{
	v2UVcoords = in_texCoord;
	gl_Position = vec4(in_position,0,1);
}