#version 430

flat in int _instanceID;

uniform vec3 gColor[64];

void main(void)
{
	gl_FragColor = vec4(gColor[_instanceID].rgb,1.0f);
}
