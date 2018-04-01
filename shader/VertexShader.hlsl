cbuffer MatrixBuffer
{
	matrix worldMatrix;
	matrix viewMatrix;
	matrix projectionMatrix;
};

struct VertexInputType 
{
	float4 position:POSITION;
	float4 color:COLOR;
};

struct PixelInputType
{
	float4 position:SV_POSITION;
	float4 color:COLOR;

};

PixelInputType VertexShaderMain(VertexInputType input)
{
	PixelInputType output;
	output.position = mul(input.position, worldMatrix);
	output.position = mul(output.position, viewMatrix);
	output.position = mul(output.position, projectionMatrix);
	output.color = input.color;

	return output;
}