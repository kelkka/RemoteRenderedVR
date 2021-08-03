//Full screen quad rendering, rectangle comes from cuda, resize and move that to 2d texture here

#version 450 core

uniform sampler2DRect mytexture;

out vec4 outputColor;

void main()
{
	ivec2 texSize = textureSize(mytexture,0);
	outputColor = vec4(texture2DRect(mytexture, vec2(gl_FragCoord.x, texSize.y - gl_FragCoord.y)).rgb, 1.0f);
}