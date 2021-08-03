
cbuffer cbExtra
{
	int gHeight;
	int pad0;
};

sampler gSampler : register(s0);
Texture2D gScene : register(t0);
Texture2D<uint2> gCompressionTexture : register(t1);

float4 main(
	float4 _pos:SV_POSITION
) : SV_TARGET
{
	int2 texCoord = int2(( _pos.x), gHeight - (_pos.y - 0.5f));

	//Get mapping coord
	int2 mapping = gCompressionTexture.Load(int3(texCoord, 0)).rg;

	//Get color from that mapped coord
	float4 color = gScene.Load(int3(mapping, 0));

	return color;
}