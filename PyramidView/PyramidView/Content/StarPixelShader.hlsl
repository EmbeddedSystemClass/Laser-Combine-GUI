#include "Fog.hlsli"

Texture2D stars	: register (t0);

sampler sampler_stars : register (s0) = sampler_state
{
	texture = NULL;
	mipfilter = ANISOTROPIC;
	minfilter = ANISOTROPIC;
	magfilter = ANISOTROPIC;
	MaxAnisotropy = 10;
};

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

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float2 texcoord : TEXCOORD0;
	float3 position : TEXCOORD1;
	float3 view : TEXCOORD2;
};

// A pass-through function for the (interpolated) color data.
float4 main(PixelShaderInput input) : SV_TARGET
{
	float3 diffuse_stars = stars.Sample(sampler_stars, input.texcoord.yx).xyz;

	float3 color = diffuse_stars * 0.0f; // exp(log(saturate(diffuse_stars - 0.1f)) * 4.0f) * 0.0f;

	float4 eye = float4(eyePos.xyz, 1.0f);
	//eye = mul(eye, model);
	//eye = mul(eye, view);
	//eye = float4(0.0f, 0.0f, 0.0f, 1.0f);

	float4 Earth = float4(float3(0.0f, 0.0f, 0.0f), 1.0f);
	Earth = mul(Earth, model);
	//Earth = mul(Earth, view);

	//float FogTransmittance = HFog(input.position.y, eyePos.y, length(input.view), fog_r0, fog_mu, fog_ro);
	float FogTransmittance = SFog(input.position, Earth.xyz, eye.xyz, fog_r0, fog_mu, fog_ro);

	float3 final_color = float3(1.0f, 1.0f, 1.0f) * (1.0f - FogTransmittance) + color * FogTransmittance;

	return float4(final_color, 1.0f);
}
