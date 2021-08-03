// fragment shader
#version 450 core

uniform sampler2D mytexture;

//uniform int width;
//uniform int height;

noperspective  in vec2 v2UVred;
noperspective  in vec2 v2UVgreen;
noperspective  in vec2 v2UVblue;

out vec4 outputColor;

void main()
{
	//vec2 fcoordsG = v2UVgreen;
	//fcoordsG.y = 1 - fcoordsG.y;

	float fBoundsCheck = 
		( 
			(
				dot
				( 
					vec2(lessThan(v2UVgreen.xy, vec2(0.05, 0.05))), vec2(1.0, 1.0)
				)
				+
				dot
				( 
					vec2(greaterThan(v2UVgreen.xy, vec2( 0.95, 0.95))), vec2(1.0, 1.0)
				)
			) 
		);

	if( fBoundsCheck > 0.9 )
	{ 
		outputColor = vec4(0, 0, 0, 1.0 );
	}
	else
	{
		//ivec2 coordsR = ivec2(v2UVred.x * width, (1-v2UVred.y) * height);
		//ivec2 coordsG = ivec2(v2UVgreen.x * width, (1-v2UVgreen.y) * height);
		//ivec2 coordsB = ivec2(v2UVblue.x * width, (1-v2UVblue.y) * height);

		//float red = texture2DRect(mytexture, coordsR).r;
		//float green = texture2DRect(mytexture, coordsG).g;
		//float blue = texture2DRect(mytexture, coordsB).b;

		float red = texture(mytexture, v2UVred).x;
		float green = texture(mytexture, v2UVgreen).y;
		float blue = texture(mytexture, v2UVblue).z;

		outputColor = vec4( red, green, blue, 1.0 ); 
	}
}
