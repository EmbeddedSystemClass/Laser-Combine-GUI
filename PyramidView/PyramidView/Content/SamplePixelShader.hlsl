Texture2DArray stars	: register (t0);

sampler sampler_stars : register (s0) = sampler_state
{
	texture = NULL;
	mipfilter = ANISOTROPIC;
	minfilter = ANISOTROPIC;
	magfilter = ANISOTROPIC;
	MaxAnisotropy = 10;
};

// Light sources
cbuffer LightSources : register(b1)
{
	float3 lightSources[16];
	float3 lightSourcesColor[16];
}

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 position : TEXCOORD0;
	float3 normal : TEXCOORD1;
	float3 view : TEXCOORD2;
	float2 texcoord : TEXCOORD3;
};

float3 CalculateTexcoord(float2 texcoord)
{
	int i = floor(texcoord.x * 4.0f);
	int j = floor(texcoord.y * 8.0f);
	uint index = i + j * 4;

	float3 tex = float3(texcoord.x * 4.0f - float(i), texcoord.y * 8.0f - float(j), index);

	return tex.yxz;
}

// A pass-through function for the (interpolated) color data.
float4 main(PixelShaderInput input) : SV_TARGET
{
	float3 light = normalize(lightSources[0] - input.position);

	float3 texcoord = CalculateTexcoord(input.texcoord);

	float4 diffuse_stars = stars.Sample(sampler_stars, texcoord);

	float diffuse = saturate(dot(input.normal, light));
	float ambient = 0.01f;

	return float4(diffuse_stars.xyz * (diffuse + ambient), diffuse_stars.w);
}
