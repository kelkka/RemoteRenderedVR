#version 450 core

layout(binding = 0) uniform sampler2D mytexture;

uniform float repeat;
uniform vec3 eyePos;
uniform float normalDirs;

in vec2 v2UVcoords;
in vec3 v3Normal;
in vec3 v3Pos;
out vec4 outputColor;

const vec3 lightPos = vec3(0, 2, 0);
const float ambient = 0.1f;

void main()
{
	outputColor = texture(mytexture, v2UVcoords * repeat);

	vec3 lightDir = normalize(lightPos - v3Pos);
	vec3 normal = normalize(v3Normal * normalDirs);

	float diffuse = max(dot(lightDir, normal), 0.0);
	float specular = 0;

	if (diffuse > 0)
	{
		vec3 toEye = normalize(eyePos - v3Pos);
		vec3 reflection = reflect(-lightDir, normal);
		specular = pow(max(dot(reflection, toEye), 0.0), 32);
	}

	float dist = length(lightPos - v3Pos);
	float attenuation = 1.0 / (1.0f + 0.1f * dist + 0.05f * (dist * dist));
	diffuse *= attenuation;
	specular *= attenuation;

	outputColor.rgb = (specular + diffuse + ambient) * outputColor.rgb;
	outputColor.rgb += 0.05f;
}