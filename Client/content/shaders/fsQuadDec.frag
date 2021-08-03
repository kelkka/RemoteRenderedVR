//Full screen quad rendering, rectangle comes from cuda, resize and move that to 2d texture here

#version 450 core

uniform sampler2DRect mytexture;

out vec4 outputColor;

void main()
{
	outputColor = vec4(texture2DRect(mytexture, gl_FragCoord.xy).rgb, 1.0f);
}