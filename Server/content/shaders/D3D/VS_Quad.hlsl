
cbuffer cbPerFrame
{
	row_major float4x4 W;
	row_major float4x4 V;
	row_major float4x4 P;
};

static const float PI = 3.14159265f;

void main(

	float3 _pos : POSITION, 
	float3 _normal : NORMAL, 
	float2 _texCoord : TEX_COORD,

	out float4 outPos : SV_POSITION,
	out float3 outNormal : NORMAL,
	out float3 outPosW : POS_WORLD,
	out float2 outTexCoord: TEX_COORD
)
{
	float4x4 WVP = mul(mul(W, V), P);
	outPos = mul(float4(_pos, 1.0f), WVP);

	outNormal = mul(float4(_normal, 0.0f), W).xyz;
	_texCoord.y = 1 - _texCoord.y;
	outTexCoord = _texCoord;

	outPosW = mul(float4(_pos, 1.0f), W).xyz;
}