#pragma once

namespace PyramidView
{
	const int lightSourcesNum = 16;

	// Constant buffer used to send MVP matrices to the vertex shader.
	struct ModelViewProjectionConstantBuffer
	{
		DirectX::XMFLOAT4X4 model;
		DirectX::XMFLOAT4X4 view;
		DirectX::XMFLOAT4X4 projection;
		DirectX::XMFLOAT4	eyePos;
	};

	struct LightSources
	{
		DirectX::XMFLOAT3	lightSources[lightSourcesNum];
		DirectX::XMFLOAT3	lightSourcesColor[lightSourcesNum];
	};

	struct ClipPlanesConstantBuffer
	{
		DirectX::XMFLOAT4	clipPlane1;
		DirectX::XMFLOAT4	clipPlane2;
	};

	// Used to send per-vertex data to the vertex shader.
	struct VertexPositionColor
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 color;
	};

	// Used to send per-vertex data to the vertex shader.
	struct VertexPositionNormalColor
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 normal;
		DirectX::XMFLOAT3 color;
		DirectX::XMFLOAT3 binormal;
		DirectX::XMFLOAT3 tangent;
		DirectX::XMFLOAT2 texCoord;
	};
}