
cbuffer cbLight: register(b0)
{
	float3 gLightColor;
	int gIsWorldMesh;
	float3 gLightPos;
	float pad1;
};

cbuffer cbModel: register(b1)
{
	row_major float4x4 P;
	row_major float4x4 V;
	row_major float4x4 M;
	float3 gEyePos;
	float gNormalsDir;
	float gRepeatTexture;
	float3 pad2;
};

sampler gSampler : register(s0);
Texture2D gTexture : register(t0);

float4 main(
	float4 _pos:SV_POSITION,
	float3 _normal : NORMAL,
	float3 _posW : POS_WORLD,
	float2 _texCoord : TEX_COORD
) : SV_TARGET
{
	float4 outputColor = gTexture.Sample(gSampler, _texCoord * gRepeatTexture);

	if(gIsWorldMesh > 0)
	{
		return outputColor;
	}

	float3 lightDir = normalize(gLightPos - _posW);
	float3 normal = normalize(_normal * gNormalsDir);

	float diffuse = max(dot(lightDir, normal), 0.0);
	float specular = 0;

	if (diffuse > 0)
	{
		float3 toEye = normalize(gEyePos - _posW);
		float3 reflection = reflect(-lightDir, normal);
		specular = pow(max(dot(reflection, toEye), 0.0), 32);
	}

	float dist = length(gLightPos - _posW);
	float attenuation = 1.0 / (1.0f + 0.1f * dist + 0.05f * (dist * dist));
	diffuse *= attenuation;
	specular *= attenuation;

	const float ambient = 0.1f;

	outputColor.rgb = (specular + diffuse + ambient) * outputColor.rgb;
	//outputColor.rgb += 0.05f;

	return outputColor;

	/////
	/*
	float4 ambient = float4(0.0f,0.0f,0.0f, 1.0f);
	float4 diffuse = float4(0.0f, 0.0f, 0.0f, 1.0f);
	float4 specular = float4(0.0f, 0.0f, 0.0f, 1.0f);

	float3 lightVec = ;

	float3 toEyeW = normalize(gEyePos - _posW);

	float dist = length(lightVec);

	float3 normal = normalize(_normal) * gNormalsDir;

	lightVec /= dist;

	float diffuseFactor = dot(lightVec, normal);
	if (diffuseFactor > 0.0f)
	{
		float3 v = reflect(-lightVec, normal);
		float specFactor = pow(max(dot(v, toEyeW), 0.0f), 16);
		specular.rgb = specFactor * float3(1.0f, 1.0f, 1.0f);
		diffuse.rgb = diffuseFactor * gLightColor;
	}
	
	float att = 1.0f / dot(float3(1.3f, 1.0f, 0.01f), float3(1.0f, dist, dist * dist));

	diffuse *= att;
	specular *= att;

	float4 color = gTexture.Sample(gSampler, _texCoord * gRepeatTexture) * (ambient + diffuse) + specular;

	color.a = 1;

	return color;

	*/
}