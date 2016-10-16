#pragma once

#include "..\Common\DeviceResources.h"
#include "ShaderStructures.h"
#include "BasicLoader.h"
#include "..\Common\StepTimer.h"

namespace PyramidView
{
	const int PyramidLeftView	= 0;
	const int PyramidRightView	= 1;
	const int PyramidFrontView	= 2;
	const int PyramidBackView	= 3;
	const int PyramidNumViews	= 4;

	// This sample renderer instantiates a basic rendering pipeline.
	class Sample3DSceneRenderer
	{
	public:
		Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		void CreateDeviceDependentResources();
		void CreateWindowSizeDependentResources();
		void ReleaseDeviceDependentResources();
		void Update(DX::StepTimer const& timer);
		void Render();
		void StartTracking();
		void TrackingUpdate(float positionX);
		void StopTracking();
		bool IsTracking() { return m_tracking; }

	private:
		void Rotate(float radians);

		void RenderEarth();
		void RenderCloud();
		void RenderStars();

	private:
		template <class DeviceChildType>
		inline void SetDebugName(
			_In_ DeviceChildType* object,
			_In_ Platform::String^ name
			);

		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		// Direct3D resources for earth geometry.
		Microsoft::WRL::ComPtr<ID3D11InputLayout>	m_inputLayout;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_indexBuffer;

		Microsoft::WRL::ComPtr<ID3D11VertexShader>	m_vertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>	m_pixelShader;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>	m_vertexShaderEarth;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>	m_pixelShaderEarth;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>	m_vertexShaderCloud;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>	m_pixelShaderCloud;

		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_constantBufferLightSources;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_constantBufferEarth[PyramidNumViews];
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_constantBufferCloud[PyramidNumViews];
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_constantBufferStars[PyramidNumViews];

		// System resources for earth geometry.
		BasicLoader^								m_loader;
		ModelViewProjectionConstantBuffer			m_constantBufferDataEarth[PyramidNumViews];
		ModelViewProjectionConstantBuffer			m_constantBufferDataCloud[PyramidNumViews];
		ModelViewProjectionConstantBuffer			m_constantBufferDataStars[PyramidNumViews];
		LightSources								m_constantBufferLightData;
		uint32										m_indexCount;

		// Textures for earth.
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>		m_earthSRV_SpaceStars;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>		m_earthSRV_DiffuseMap;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>		m_earthSRV_SpecularMap;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>		m_earthSRV_NormalMap;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>		m_earthSRV_NightLightMap;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>		m_earthSRV_CloudsMap;
		uint32		m_tailRowCount;
		uint32		m_tailColCount;
		float		m_fEarthRadius;
		float		m_fEarthAtmosphereThickness;

		// Sky matrix
		DirectX::XMFLOAT4X4	m_matSky;
		DirectX::XMFLOAT4X4	m_matCloud;

		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	m_earthShaderDiffuseViews;

		// Variables used with the rendering loop.
		bool	m_loadingComplete;
		float	m_degreesPerSecond;
		bool	m_tracking;
	};
}

