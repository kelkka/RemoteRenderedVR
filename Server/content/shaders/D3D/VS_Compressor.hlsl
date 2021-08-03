void main(
	float2 inPos : POSITION, 
	out float4 outPos : SV_POSITION
)
{
	outPos = float4(inPos, 0, 1);
}