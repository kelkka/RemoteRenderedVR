#version 430
layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_texCoord;

uniform vec3 gPos[64];
uniform vec2 gScale[64];
uniform mat4 gViewProj;

flat out int _instanceID;

void main(void)
{
	_instanceID = gl_InstanceID;

	mat4 modelMatrix = mat4(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		gPos[_instanceID].x, gPos[_instanceID].y, gPos[_instanceID].z, 1.0f);

	mat4 scaleMatrix = mat4(
		gScale[_instanceID].x, 0.0f, 0.0f, 0.0f,
		0.0f, gScale[_instanceID].y, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);

	gl_Position = gViewProj * modelMatrix * scaleMatrix * vec4(in_position, 0.0, 1.0);
}
