// A constant buffer that stores the three basic column-major matrices for composing geometry.
cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
	float4 eyePos;
};

// Light sources
cbuffer LightSources : register(b1)
{
	float3 lightSources[16];
	float3 lightSourcesColor[16];
}

// Clip planes
cbuffer ClipPlanesConstantBuffer : register(b2)
{
	float4 clipPlane1;
	float4 clipPlane2;
}

// Per-vertex data used as input to the vertex shader.
struct VertexShaderInput
{
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float3 color : COLOR0;
	float3 binormal : BINORMAL;
	float3 tangent : TANGENT;
	float2 tex : TEXCOORD0;
};

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float2 texcoord : TEXCOORD0;
	float3 position : TEXCOORD1;
	float3 view : TEXCOORD2;
};

// Simple shader to do vertex processing on the GPU.
[clipplanes(clipPlane1, clipPlane2)]
PixelShaderInput main(VertexShaderInput input)
{
	PixelShaderInput output;
	float4 pos = float4(input.pos, 1.0f);

	// Transform the vertex position into projection space.
	pos = mul(pos, model);
	output.position = pos;
	pos = mul(pos, view);
	pos = mul(pos, projection);
	output.pos = pos;
	output.view = eyePos.xyz - pos.xyz;

	// Pass the color through without modification.
	output.texcoord = input.tex;

	return output;
}
