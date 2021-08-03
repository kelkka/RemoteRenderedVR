
cbuffer cbModel: register(b0)
{
	row_major float4x4 P;
	row_major float4x4 V;
	row_major float4x4 M;
	float3 EyePos;
	float NormalsDir;
};

static const float PI = 3.14159265f;

void main(

	float3 _pos : POSITION, 
	float2 _texCoord : TEX_COORD,
	float3 _normal : NORMAL,

	out float4 outPos : SV_POSITION,
	out float3 outNormal : NORMAL,
	out float3 outPosW : POS_WORLD,
	out float2 outTexCoord: TEX_COORD
)
{
	float4x4 MVP = mul(mul(M, V), P);
	outPos = mul(float4(_pos, 1.0f), MVP);

	outNormal = normalize(mul(float4(_normal, 0.0f), M).xyz);
	_texCoord.y = 1 - _texCoord.y;
	outTexCoord = _texCoord;

	outPosW = mul(float4(_pos, 1.0f), M).xyz;
}