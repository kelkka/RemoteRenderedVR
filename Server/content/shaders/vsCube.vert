#version 430

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 v2UVcoordsIn;
layout(location = 2) in vec3 v3NormalIn;

out vec2 v2UVcoords;
out vec3 v3Normal;
out vec3 v3Pos;

uniform mat4 PVM;
uniform mat4 matrixModel;

void main()
{
	v2UVcoords = v2UVcoordsIn;
	v2UVcoords.y = 1 - v2UVcoords.y;
	v3Normal = vec4(matrixModel * vec4(v3NormalIn,0.0f)).xyz;
	gl_Position = PVM * position;
	v3Pos = vec4(matrixModel * position).xyz;
}