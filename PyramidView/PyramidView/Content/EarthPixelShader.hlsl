#include "Fog.hlsli"

Texture2DArray earth_diffuse	: register (t0);
Texture2DArray earth_specular	: register (t1);
Texture2DArray earth_bumpmap	: register (t2);
Texture2DArray earth_nightlight	: register (t3);
Texture2DArray earth_clouds		: register (t4);

sampler sampler_earth : register (s0) = sampler_state
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
	float3 position : TEXCOORD0;
	float3 normal : TEXCOORD1;
	float3 binormal : TEXCOORD2;
	float3 tangent : TEXCOORD3;
	float3 view : TEXCOORD4;
	float2 texcoord : TEXCOORD5;
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
	// calculate texcoord
	float3 texcoord = CalculateTexcoord(input.texcoord);
	//float3 texcoord_cloud = CalculateTexcoord(input.texcoord + float2());

	// sample textures
	float4 diffuse_tex = earth_diffuse.Sample(sampler_earth, texcoord);
	float4 specular_tex = earth_specular.Sample(sampler_earth, texcoord);
	float4 clouds_tex = earth_clouds.Sample(sampler_earth, texcoord);
	float4 night_tex = earth_nightlight.Sample(sampler_earth, texcoord);
	float3 normal = earth_bumpmap.Sample(sampler_earth, texcoord);
	normal = normalize(normal * 2.0f - 1.0f);

	// tangent space matrix
	float3x3 tangentSpace;
	tangentSpace[1] = input.tangent;
	tangentSpace[0] = input.binormal;
	tangentSpace[2] = input.normal;

	// calculate light
	float3 light = normalize(mul(tangentSpace, normalize(lightSources[0] - input.position)));
	float3 view  = normalize(mul(tangentSpace, input.view));
	float3 light_refl = -reflect(light, normal);

	float diffuse = saturate(dot(normal, light));
	float specular = saturate(dot(light_refl, view));
	float ambient = 0.01f;

	diffuse *= (1.0f - clouds_tex.w /* (1.0f - diffuse)*/);

	float3 color =
		ambient * (diffuse_tex.xyz + clouds_tex.xyz) +
		(1.0f - diffuse) * night_tex.xyz * 2.0f +
		diffuse * (diffuse_tex.xyz * (1.0f - clouds_tex.w)) +
		specular_tex.x * diffuse * 10.0f * exp(log(specular) * 100.0f);

	float4 eye = float4(eyePos.xyz, 1.0f);
	//eye = mul(eye, model);
	//eye = mul(eye, view);
	//eye = float4(0.0f, 0.0f, 0.0f, 1.0f);

	float4 Earth = float4(float3(0.0f, 0.0f, 0.0f), 1.0f);
	Earth = mul(Earth, model);
	//Earth = mul(Earth, view);

	//float FogTransmittance = HFog(input.position.y, eyePos.y, length(input.view), fog_r0, fog_mu, fog_ro);
	float FogTransmittance = SFog(input.position, Earth.xyz, eye.xyz, fog_r0, fog_mu, fog_ro);

	float3 final_color = float3(1.0f, 1.0f, 1.0f) * diffuse * (1.0f - FogTransmittance) + color * FogTransmittance;

	return float4(final_color.xyz, 1.0f);
}
