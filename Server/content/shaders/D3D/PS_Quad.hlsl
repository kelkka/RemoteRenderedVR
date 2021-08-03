
cbuffer cbLight
{
	float3 gEyePos;
	float pad0;
	float3 gLightColor;
	float pad1;
	float3 gLightPos;
	float gLightRange;
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
	float4 ambient = float4(0.0f,0.0f,0.0f, 1.0f);
	float4 diffuse = float4(0.0f, 0.0f, 0.0f, 1.0f);
	float4 specular = float4(0.0f, 0.0f, 0.0f, 1.0f);

	float3 lightVec = gLightPos - _posW;

	float3 toEyeW = normalize(gEyePos - _posW);

	float dist = length(lightVec);

	float3 normal = normalize(_normal);

	if (dist < gLightRange)
	{
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
	}

	float4 color = gTexture.Sample(gSampler, _texCoord) * (ambient + diffuse) + specular;

	color.a = 1;

	return color;
}