//struct PixelShaderInput
//{
//	float4 pos : SV_POSITION;
//	float4 diffuse : COLOR;
//	float2 uv : TEXCOORD0;
//	float3 worldNorm : TEXCOORD1;
//	float3 worldPos : TEXCOORD2;
//	float3 toEye : TEXCOORD3;
//	float4 tangent : TEXCOORD4;
//	float3 normal : TEXCOORD5;
//};


struct PixelInputType
{
	float4 position:SV_POSITION;
	float4 color:COLOR;

};

float4 PixelShaderMain(PixelInputType input) : SV_TARGET
{

	return input.color;
}