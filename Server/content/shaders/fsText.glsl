#version 430

flat in int _instanceID;
in vec2 texCoord;

uniform sampler2D gTexture;

uniform vec3 gColor[64];

void main(void)
{
	vec4 outColor = texture(gTexture, texCoord);
	
	gl_FragColor = outColor;
}
