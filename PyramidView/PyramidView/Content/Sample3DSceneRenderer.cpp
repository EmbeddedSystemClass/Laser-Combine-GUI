#include "pch.h"
#include "Sample3DSceneRenderer.h"

#include "..\Common\DirectXHelper.h"

using namespace PyramidView;

using namespace DirectX;
using namespace Windows::Foundation;
using namespace Microsoft::WRL;

//#define PIRAMID_VIEW

template <class DeviceChildType>
inline void Sample3DSceneRenderer::SetDebugName(
	_In_ DeviceChildType* object,
	_In_ Platform::String^ name
	)
{
#if defined(_DEBUG)
	// Only assign debug names in debug builds.

	char nameString[1024];
	int nameStringLength = WideCharToMultiByte(
		CP_ACP,
		0,
		name->Data(),
		-1,
		nameString,
		1024,
		nullptr,
		nullptr
		);

	if (nameStringLength == 0)
	{
		char defaultNameString[] = "Sample3DSceneRenderer";
		DX::ThrowIfFailed(
			object->SetPrivateData(
				WKPDID_D3DDebugObjectName,
				sizeof(defaultNameString) - 1,
				defaultNameString
				)
			);
	}
	else
	{
		DX::ThrowIfFailed(
			object->SetPrivateData(
				WKPDID_D3DDebugObjectName,
				nameStringLength - 1,
				nameString
				)
			);
	}
#endif
}

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
Sample3DSceneRenderer::Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_loadingComplete(false),
	m_degreesPerSecond(1),
	m_indexCount(0),
	m_tracking(false),
	m_deviceResources(deviceResources),
	m_tailRowCount(8),
	m_tailColCount(4),
	m_fEarthRadius(1.0f),
	m_fEarthAtmosphereThickness(0.002f)
{
	// Create resource loader
	m_loader = ref new BasicLoader(deviceResources->GetD3DDevice());

	//m_earthShaderResourceViews.resize(m_tailRowCount * m_tailColCount);
	//m_earthTextures2D.resize(m_tailRowCount * m_tailColCount);

	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}

// Initializes view parameters when the window size changes.
void Sample3DSceneRenderer::CreateWindowSizeDependentResources()
{
	Size outputSize = m_deviceResources->GetOutputSize();
	float aspectRatio = outputSize.Width / outputSize.Height;
	float fovAngleY = 45.0f * XM_PI / 180.0f;

	// This is a simple example of change that can be made when the app is in
	// portrait or snapped view.
	if (aspectRatio < 1.0f)
	{
		fovAngleY *= 2.0f;
	}

	// Note that the OrientationTransform3D matrix is post-multiplied here
	// in order to correctly orient the scene to match the display orientation.
	// This post-multiplication step is required for any draw calls that are
	// made to the swap chain render target. For draw calls to other targets,
	// this transform should not be applied.

	// This sample makes use of a right-handed coordinate system using row-major matrices.
	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovRH(
		fovAngleY,
		aspectRatio,
		0.01f,
		100.0f
		);

	XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();

	XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);

	XMMATRIX proj = XMMatrixTranspose(perspectiveMatrix * orientationMatrix);

	XMMATRIX PLV = XMMatrixTranspose(XMMatrixTranslation(-0.6f / aspectRatio, 0.0f, 0.0f)) * proj;
	XMMATRIX PRV = XMMatrixTranspose(XMMatrixTranslation(0.6f / aspectRatio, 0.0f, 0.0f))* proj;

#ifdef PIRAMID_VIEW
	XMMATRIX PFV = XMMatrixTranspose(XMMatrixTranslation(0.0f, 0.6f, 0.0f)) * proj;
#else
	XMMATRIX PFV = proj;
#endif

	XMMATRIX PBV = XMMatrixTranspose(XMMatrixTranslation(0.0f, -0.6f, 0.0f))* proj;

	XMStoreFloat4x4(&m_constantBufferDataEarth[PyramidLeftView].projection, PLV);
	XMStoreFloat4x4(&m_constantBufferDataEarth[PyramidRightView].projection, PRV);
	XMStoreFloat4x4(&m_constantBufferDataEarth[PyramidFrontView].projection, PFV);
	XMStoreFloat4x4(&m_constantBufferDataEarth[PyramidBackView].projection, PBV);

	XMStoreFloat4x4(&m_constantBufferDataCloud[PyramidLeftView].projection, PLV);
	XMStoreFloat4x4(&m_constantBufferDataCloud[PyramidRightView].projection, PRV);
	XMStoreFloat4x4(&m_constantBufferDataCloud[PyramidFrontView].projection, PFV);
	XMStoreFloat4x4(&m_constantBufferDataCloud[PyramidBackView].projection, PBV);

	XMStoreFloat4x4(&m_constantBufferDataStars[PyramidLeftView].projection, PLV);
	XMStoreFloat4x4(&m_constantBufferDataStars[PyramidRightView].projection, PRV);
	XMStoreFloat4x4(&m_constantBufferDataStars[PyramidFrontView].projection, PFV);
	XMStoreFloat4x4(&m_constantBufferDataStars[PyramidBackView].projection, PBV);

	// Eye is at (0,0.7,1.5), looking at point (0,-0.1,0) with the up-vector along the y-axis.
	static XMVECTORF32 at = { 0.0f, 0.0f, 0.0f, 0.0f };

	static XMVECTORF32 eye_LV = {-8.0f, 0.0f, 0.0f, 0.0f };
	static XMVECTORF32 eye_RV = { 8.0f, 0.0f, 0.0f, 0.0f };
#ifdef	PIRAMID_VIEW
	static XMVECTORF32 eye_FV = { 0.0f, 0.0f, 8.0f, 0.0f }; // replace 2.0f with 6.0f
#else
	static XMVECTORF32 eye_FV = { 0.0f, 0.0f, 4.0f, 0.0f }; // replace 2.0f with 6.0f
#endif

	static XMVECTORF32 eye_BV = { 0.0f, 0.0f,-8.0f, 0.0f };
	static XMVECTORF32 up_LV  = { 0.0f, 0.0f, 1.0f, 0.0f };
	static XMVECTORF32 up_RV  = { 0.0f, 0.0f, 1.0f, 0.0f };
	static XMVECTORF32 up_FV  = { 0.0f, 1.0f, 0.0f, 0.0f };
	static XMVECTORF32 up_BV  = { 0.0f,-1.0f, 0.0f, 0.0f };

	XMStoreFloat4(&m_constantBufferDataEarth[PyramidLeftView].eyePos, eye_LV);
	XMStoreFloat4(&m_constantBufferDataEarth[PyramidRightView].eyePos, eye_RV);
	XMStoreFloat4(&m_constantBufferDataEarth[PyramidFrontView].eyePos, eye_FV);
	XMStoreFloat4(&m_constantBufferDataEarth[PyramidBackView].eyePos, eye_BV);

	XMStoreFloat4(&m_constantBufferDataCloud[PyramidLeftView].eyePos, eye_LV);
	XMStoreFloat4(&m_constantBufferDataCloud[PyramidRightView].eyePos, eye_RV);
	XMStoreFloat4(&m_constantBufferDataCloud[PyramidFrontView].eyePos, eye_FV);
	XMStoreFloat4(&m_constantBufferDataCloud[PyramidBackView].eyePos, eye_BV);

	XMStoreFloat4(&m_constantBufferDataStars[PyramidLeftView].eyePos, eye_LV);
	XMStoreFloat4(&m_constantBufferDataStars[PyramidRightView].eyePos, eye_RV);
	XMStoreFloat4(&m_constantBufferDataStars[PyramidFrontView].eyePos, eye_FV);
	XMStoreFloat4(&m_constantBufferDataStars[PyramidBackView].eyePos, eye_BV);

	XMStoreFloat4x4(&m_constantBufferDataEarth[PyramidLeftView].view, XMMatrixTranspose(XMMatrixLookAtRH(eye_LV, at, up_LV)));
	XMStoreFloat4x4(&m_constantBufferDataEarth[PyramidRightView].view, XMMatrixTranspose(XMMatrixLookAtRH(eye_RV, at, up_RV)));
	XMStoreFloat4x4(&m_constantBufferDataEarth[PyramidFrontView].view, XMMatrixTranspose(XMMatrixLookAtRH(eye_FV, at, up_FV)));
	XMStoreFloat4x4(&m_constantBufferDataEarth[PyramidBackView].view, XMMatrixTranspose(XMMatrixLookAtRH(eye_BV, at, up_BV)));

	XMStoreFloat4x4(&m_constantBufferDataCloud[PyramidLeftView].view, XMMatrixTranspose(XMMatrixLookAtRH(eye_LV, at, up_LV)));
	XMStoreFloat4x4(&m_constantBufferDataCloud[PyramidRightView].view, XMMatrixTranspose(XMMatrixLookAtRH(eye_RV, at, up_RV)));
	XMStoreFloat4x4(&m_constantBufferDataCloud[PyramidFrontView].view, XMMatrixTranspose(XMMatrixLookAtRH(eye_FV, at, up_FV)));
	XMStoreFloat4x4(&m_constantBufferDataCloud[PyramidBackView].view, XMMatrixTranspose(XMMatrixLookAtRH(eye_BV, at, up_BV)));

	XMStoreFloat4x4(&m_constantBufferDataStars[PyramidLeftView].view, XMMatrixTranspose(XMMatrixLookAtRH(XMVectorSet(-0.1f, 0.0f, 0.0f, 0.0f), at, up_LV)));
	XMStoreFloat4x4(&m_constantBufferDataStars[PyramidRightView].view, XMMatrixTranspose(XMMatrixLookAtRH(XMVectorSet(0.1f, 0.0f, 0.0f, 0.0f), at, up_RV)));
#ifdef	PIRAMID_VIEW
	XMStoreFloat4x4(&m_constantBufferDataStars[PyramidFrontView].view, XMMatrixTranspose(XMMatrixLookAtRH(XMVectorSet(0.0f, 0.0f, 0.1f, 0.0f), at, up_FV)));
#else
	XMStoreFloat4x4(&m_constantBufferDataStars[PyramidFrontView].view, XMMatrixTranspose(XMMatrixLookAtRH(eye_FV, at, up_FV)));
#endif
	XMStoreFloat4x4(&m_constantBufferDataStars[PyramidBackView].view, XMMatrixTranspose(XMMatrixLookAtRH(XMVectorSet(0.0f, 0.0f, -0.1f, 0.0f), at, up_BV)));
}

// Called once per frame, rotates the cube and calculates the model and view matrices.
void Sample3DSceneRenderer::Update(DX::StepTimer const& timer)
{
	if (!m_tracking)
	{
		// First light source
		m_constantBufferLightData.lightSources[0] = XMFLOAT3(0.0f, 0.0f, 50.0f);
		m_constantBufferLightData.lightSourcesColor[0] = XMFLOAT3(1.0f, 1.0f, 1.0f);

		// Convert degrees to radians, then convert seconds to rotation angle
		float radiansPerSecond = XMConvertToRadians(m_degreesPerSecond);
		double totalRotation = timer.GetTotalSeconds() * radiansPerSecond;
		float radians = static_cast<float>(fmod(totalRotation, XM_2PI));

		Rotate(radians);
	}
}

// Rotate the 3D cube model a set amount of radians.
void Sample3DSceneRenderer::Rotate(float radians)
{
	// Prepare to pass the updated model matrix to the shader
	for (UINT i = 0; i < PyramidNumViews; i++)
	{
		XMStoreFloat4x4(&m_constantBufferDataEarth[i].model, XMMatrixScaling(1.0f, 1.0f, 1.0f) * XMMatrixTranspose(XMMatrixRotationY(radians)));
		XMStoreFloat4x4(&m_constantBufferDataCloud[i].model, XMMatrixScaling(
			(m_fEarthRadius + m_fEarthAtmosphereThickness) / m_fEarthRadius, 
			(m_fEarthRadius + m_fEarthAtmosphereThickness) / m_fEarthRadius,
			(m_fEarthRadius + m_fEarthAtmosphereThickness) / m_fEarthRadius
			) * XMMatrixTranspose(XMMatrixRotationY(radians)));
		XMStoreFloat4x4(&m_constantBufferDataStars[i].model, XMMatrixScaling(50.0f, -50.0f, 50.0f) * XMMatrixTranspose(XMMatrixRotationY(radians)));

		//m_constantBufferDataEarth[i].eyePos.z = 3.0f + 3.0f * (1.0 + cos(radians*10.0f));
		//m_constantBufferDataStars[i].eyePos.z = 3.0f + 3.0f * (1.0 + cos(radians*10.0f));
	}

	XMVECTOR light = XMLoadFloat3(&m_constantBufferLightData.lightSources[0]);
	XMVECTOR pos = XMVector3TransformCoord(light, XMMatrixRotationY(radians * -0.5f));
	XMStoreFloat3(&m_constantBufferLightData.lightSources[0], pos);
}

void Sample3DSceneRenderer::StartTracking()
{
	m_tracking = true;
}

// When tracking, the 3D cube can be rotated around its Y axis by tracking pointer position relative to the output screen width.
void Sample3DSceneRenderer::TrackingUpdate(float positionX)
{
	if (m_tracking)
	{
		float radians = XM_2PI * 2.0f * positionX / m_deviceResources->GetOutputSize().Width;
		Rotate(radians);
	}
}

void Sample3DSceneRenderer::StopTracking()
{
	m_tracking = false;
}

void Sample3DSceneRenderer::RenderEarth()
{
	auto context = m_deviceResources->GetD3DDeviceContext();

	// Prepare the constant buffer to send it to the graphics device.
	// Earth constant buffer
	context->UpdateSubresource(m_constantBufferEarth[PyramidLeftView].Get(), 0, NULL, &m_constantBufferDataEarth[PyramidLeftView], 0, 0);
	context->UpdateSubresource(m_constantBufferEarth[PyramidRightView].Get(), 0, NULL, &m_constantBufferDataEarth[PyramidRightView], 0, 0);
	context->UpdateSubresource(m_constantBufferEarth[PyramidFrontView].Get(), 0, NULL, &m_constantBufferDataEarth[PyramidFrontView], 0, 0);
	context->UpdateSubresource(m_constantBufferEarth[PyramidBackView].Get(), 0, NULL, &m_constantBufferDataEarth[PyramidBackView], 0, 0);

	// Each vertex is one instance of the VertexPositionNormalColor struct.
	{
		UINT stride = sizeof(VertexPositionNormalColor);
		UINT offset = 0;
		context->IASetVertexBuffers(
			0,
			1,
			m_vertexBuffer.GetAddressOf(),
			&stride,
			&offset
			);

		context->IASetIndexBuffer(
			m_indexBuffer.Get(),
			DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
			0
			);

		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		context->IASetInputLayout(m_inputLayout.Get());
	}

	// Attach our vertex shader.
	context->VSSetShader(
		m_vertexShaderEarth.Get(),
		nullptr,
		0
		);

	// Attach our pixel shader.
	context->PSSetShader(
		m_pixelShaderEarth.Get(),
		nullptr,
		0
		);

	// Set textures
	context->PSSetShaderResources(0, 1, m_earthSRV_DiffuseMap.GetAddressOf());
	context->PSSetShaderResources(1, 1, m_earthSRV_SpecularMap.GetAddressOf());
	context->PSSetShaderResources(2, 1, m_earthSRV_NormalMap.GetAddressOf());
	context->PSSetShaderResources(3, 1, m_earthSRV_NightLightMap.GetAddressOf());
	context->PSSetShaderResources(4, 1, m_earthSRV_CloudsMap.GetAddressOf());

	UINT i = 2;

#ifdef PIRAMID_VIEW
	for (UINT i = 0; i < PyramidNumViews; i++)
#endif
	{
		// Send the constant buffer to the graphics device.
		context->VSSetConstantBuffers(
			0,
			1,
			m_constantBufferEarth[i].GetAddressOf()
			);

		context->PSSetConstantBuffers(
			0,
			1,
			m_constantBufferEarth[i].GetAddressOf()
			);

		// Draw the objects.
		context->DrawIndexed(
			m_indexCount,
			0,
			0
			);
		//context->Draw(m_indexCount, 0);
	}
}

void Sample3DSceneRenderer::RenderCloud()
{
	auto context = m_deviceResources->GetD3DDeviceContext();

	// Prepare the constant buffer to send it to the graphics device.
	// Clouds constant buffer
	context->UpdateSubresource(m_constantBufferCloud[PyramidLeftView].Get(), 0, NULL, &m_constantBufferDataCloud[PyramidLeftView], 0, 0);
	context->UpdateSubresource(m_constantBufferCloud[PyramidRightView].Get(), 0, NULL, &m_constantBufferDataCloud[PyramidRightView], 0, 0);
	context->UpdateSubresource(m_constantBufferCloud[PyramidFrontView].Get(), 0, NULL, &m_constantBufferDataCloud[PyramidFrontView], 0, 0);
	context->UpdateSubresource(m_constantBufferCloud[PyramidBackView].Get(), 0, NULL, &m_constantBufferDataCloud[PyramidBackView], 0, 0);

	// Each vertex is one instance of the VertexPositionNormalColor struct.
	{
		UINT stride = sizeof(VertexPositionNormalColor);
		UINT offset = 0;
		context->IASetVertexBuffers(
			0,
			1,
			m_vertexBuffer.GetAddressOf(),
			&stride,
			&offset
			);

		context->IASetIndexBuffer(
			m_indexBuffer.Get(),
			DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
			0
			);

		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		context->IASetInputLayout(m_inputLayout.Get());
	}

	// Attach our vertex shader.
	context->VSSetShader(
		m_vertexShaderCloud.Get(),
		nullptr,
		0
		);

	// Attach our pixel shader.
	context->PSSetShader(
		m_pixelShaderCloud.Get(),
		nullptr,
		0
		);

	// set textures
	context->PSSetShaderResources(0, 1, m_earthSRV_CloudsMap.GetAddressOf());

	UINT i = 2;
#ifdef PIRAMID_VIEW
	for (UINT i = 0; i < PyramidNumViews; i++)
#endif
	{
		// Send the constant buffer to the graphics device.
		context->VSSetConstantBuffers(
			0,
			1,
			m_constantBufferCloud[i].GetAddressOf()
			);

		// Draw the objects.
		context->DrawIndexed(
			m_indexCount,
			0,
			0
			);
		//context->Draw(m_indexCount, 0);
	}
}

void Sample3DSceneRenderer::RenderStars()
{
	auto context = m_deviceResources->GetD3DDeviceContext();

	// Prepare the constant buffer to send it to the graphics device.
	// Stars constant buffer
	context->UpdateSubresource(m_constantBufferStars[PyramidLeftView].Get(), 0, NULL, &m_constantBufferDataStars[PyramidLeftView], 0, 0);
	context->UpdateSubresource(m_constantBufferStars[PyramidRightView].Get(), 0, NULL, &m_constantBufferDataStars[PyramidRightView], 0, 0);
	context->UpdateSubresource(m_constantBufferStars[PyramidFrontView].Get(), 0, NULL, &m_constantBufferDataStars[PyramidFrontView], 0, 0);
	context->UpdateSubresource(m_constantBufferStars[PyramidBackView].Get(), 0, NULL, &m_constantBufferDataStars[PyramidBackView], 0, 0);

	//m_earthSRV_SpaceStars

	// Each vertex is one instance of the VertexPositionNormalColor struct.
	{
		UINT stride = sizeof(VertexPositionNormalColor);
		UINT offset = 0;
		context->IASetVertexBuffers(
			0,
			1,
			m_vertexBuffer.GetAddressOf(),
			&stride,
			&offset
			);

		context->IASetIndexBuffer(
			m_indexBuffer.Get(),
			DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
			0
			);

		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		context->IASetInputLayout(m_inputLayout.Get());
	}

	// Attach our vertex shader.
	context->VSSetShader(
		m_vertexShader.Get(),
		nullptr,
		0
		);

	// Attach our pixel shader.
	context->PSSetShader(
		m_pixelShader.Get(),
		nullptr,
		0
		);

	// set textures
	context->PSSetShaderResources(0, 1, m_earthSRV_SpaceStars.GetAddressOf());

	m_deviceResources->ZDisable();

	UINT i = 2;
#ifdef PIRAMID_VIEW
	for (UINT i = 0; i < PyramidNumViews; i++)
#endif
	{
		// Send the constant buffer to the graphics device.
		context->VSSetConstantBuffers(
			0,
			1,
			m_constantBufferStars[i].GetAddressOf()
			);

		// Send the constant buffer to the graphics device.
		context->PSSetConstantBuffers(
			0,
			1,
			m_constantBufferStars[i].GetAddressOf()
			);

		// Draw the objects.
		context->DrawIndexed(
			m_indexCount,
			0,
			0
			);
		//context->Draw(m_indexCount, 0);
	}

	m_deviceResources->ZEnable();
}

// Renders one frame using the vertex and pixel shaders.
void Sample3DSceneRenderer::Render()
{
	// Loading is asynchronous. Only draw geometry after it's loaded.
	if (!m_loadingComplete)
	{
		return;
	}

	auto context = m_deviceResources->GetD3DDeviceContext();

	// Light sources update
	context->UpdateSubresource(	m_constantBufferLightSources.Get(), 0, NULL, &m_constantBufferLightData, 0, 0);

	// Set constant buffers for light sources
	context->VSSetConstantBuffers(
		1,
		1,
		m_constantBufferLightSources.GetAddressOf()
		);

	context->PSSetConstantBuffers(
		1,
		1,
		m_constantBufferLightSources.GetAddressOf()
		);
#ifndef PIRAMID_VIEW
	RenderStars();
#endif
	RenderEarth();
	RenderCloud();
}

void Sample3DSceneRenderer::CreateDeviceDependentResources()
{
	static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 60, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	// Load shaders asynchronously.
	std::vector<concurrency::task<void>> loadShaderTasks(6);

	// Load graphics resources
	auto loadResourcesTask = Concurrency::create_task([this](){

		// Load textures synchronously.
		/*
		m_loader->LoadTextureArray("Earth\\earth-unshaded-hs\\textures\\medres\\earth-unshaded-hs-32k\\level2", m_tailRowCount, m_tailColCount, nullptr, m_earthSRV_DiffuseMap.GetAddressOf());
		m_loader->LoadTextureArray("Earth\\earth-spec-hs\\textures\\medres\\earth-spec-hs-32k\\level2", m_tailRowCount, m_tailColCount, nullptr, m_earthSRV_SpecularMap.GetAddressOf());
		m_loader->LoadTextureArray("Earth\\earth-normals-hs\\textures\\medres\\earth-normals-hs-32k\\level2", m_tailRowCount, m_tailColCount, nullptr, m_earthSRV_NormalMap.GetAddressOf());
		m_loader->LoadTextureArray("Earth\\earth-nightlights-hs\\textures\\medres\\earth-nightlights-hs-32k\\level2", m_tailRowCount, m_tailColCount, nullptr, m_earthSRV_NightLightMap.GetAddressOf());
		m_loader->LoadTextureArray("Earth\\earth-clouds-hs\\textures\\hires\\earth-clouds-hs-32k\\level2", m_tailRowCount, m_tailColCount, nullptr, m_earthSRV_CloudsMap.GetAddressOf());*/

		m_loader->LoadTextureArray("Media\\diffuse_", m_tailRowCount, m_tailColCount, nullptr, m_earthSRV_DiffuseMap.GetAddressOf());
		m_loader->LoadTextureArray("Media\\spec_", m_tailRowCount, m_tailColCount, nullptr, m_earthSRV_SpecularMap.GetAddressOf());
		m_loader->LoadTextureArray("Media\\normal_", m_tailRowCount, m_tailColCount, nullptr, m_earthSRV_NormalMap.GetAddressOf());
		m_loader->LoadTextureArray("Media\\nightlight_", m_tailRowCount, m_tailColCount, nullptr, m_earthSRV_NightLightMap.GetAddressOf());
		m_loader->LoadTextureArray("Media\\clouds_", m_tailRowCount, m_tailColCount, nullptr, m_earthSRV_CloudsMap.GetAddressOf());

		// load stars texture
		m_loader->LoadTexture("Media\\skymap_.dds", nullptr, &m_earthSRV_SpaceStars);

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		for (UINT i = 0; i < PyramidNumViews; i++)
		{
			DX::ThrowIfFailed(
				m_deviceResources->GetD3DDevice()->CreateBuffer(
					&constantBufferDesc,
					nullptr,
					&m_constantBufferEarth[i]
					)
				);

			DX::ThrowIfFailed(
				m_deviceResources->GetD3DDevice()->CreateBuffer(
					&constantBufferDesc,
					nullptr,
					&m_constantBufferCloud[i]
					)
				);

			DX::ThrowIfFailed(
				m_deviceResources->GetD3DDevice()->CreateBuffer(
					&constantBufferDesc,
					nullptr,
					&m_constantBufferStars[i]
					)
				);
		}

		constantBufferDesc = CD3D11_BUFFER_DESC(sizeof(LightSources), D3D11_BIND_CONSTANT_BUFFER);

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&constantBufferDesc,
				nullptr,
				&m_constantBufferLightSources
				)
			);
	});

	loadShaderTasks[0] = m_loader->LoadShaderAsync("EarthVertexShader.cso", (D3D11_INPUT_ELEMENT_DESC*)&vertexDesc[0], ARRAYSIZE(vertexDesc), &m_vertexShaderEarth, &m_inputLayout);
	loadShaderTasks[1] = m_loader->LoadShaderAsync("EarthPixelShader.cso", &m_pixelShaderEarth);
	loadShaderTasks[2] = m_loader->LoadShaderAsync("SampleVertexShader.cso", (D3D11_INPUT_ELEMENT_DESC*)&vertexDesc[0], ARRAYSIZE(vertexDesc), &m_vertexShaderCloud, nullptr);
	loadShaderTasks[3] = m_loader->LoadShaderAsync("SamplePixelShader.cso", &m_pixelShaderCloud);
	loadShaderTasks[4] = m_loader->LoadShaderAsync("StarVertexShader.cso", (D3D11_INPUT_ELEMENT_DESC*)&vertexDesc[0], ARRAYSIZE(vertexDesc), &m_vertexShader, nullptr);
	loadShaderTasks[5] = m_loader->LoadShaderAsync("StarPixelShader.cso", &m_pixelShader);

	// Once both shaders are loaded, create the mesh.
	auto createEarthTask = (Concurrency::when_all(loadShaderTasks.begin(), loadShaderTasks.end()) && loadResourcesTask).then([this] () {

		// Load mesh vertices. Each vertex has a position and a color.
		UINT numSegments = 256;
		float earthRadius = m_fEarthRadius;

		std::vector<VertexPositionNormalColor> earthVertices(numSegments * numSegments);

		for (UINT i = 0; i < numSegments; i ++)
			for (UINT j = 0; j < numSegments; j++)
			{
				int index = i + j * numSegments;
				float a = 2.0f * XM_PI * float(j) / float(numSegments-1);
				float b = XM_PI * float(i) / float(numSegments-1);

				// normal
				float nx = cosf(a) * sinf(b);
				float nz = sinf(a) * sinf(b);
				float ny = cosf(b);

				// position
				float x = earthRadius * nx;
				float y = earthRadius * ny;
				float z = earthRadius * nz;

				// texcoord
				float u = float(i) / float(numSegments - 1);
				float v = (1.0f - float(j) / float(numSegments - 1));

				// vertex
				earthVertices[index].pos		= XMFLOAT3(x, y, z);
				earthVertices[index].normal		= XMFLOAT3(nx, ny, nz);
				earthVertices[index].color		= XMFLOAT3(1.0f, 1.0f, 1.0f);
				earthVertices[index].texCoord	= XMFLOAT2(u, v);

				// calculate binormal & tangent
				XMVECTOR normal	= XMLoadFloat3(&earthVertices[index].normal);
				XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
				XMVECTOR binormal = XMVector3Cross(Up, normal);
				XMVECTOR tangent = XMVector3Cross(binormal, normal);

				// store tangent space to vertex
				XMStoreFloat3(&earthVertices[index].binormal, binormal);
				XMStoreFloat3(&earthVertices[index].tangent, tangent);
			}

		// Load mesh indices. Each trio of indices represents
		// a triangle to be rendered on the screen.
		// For example: 0,2,1 means that the vertices with indexes
		// 0, 2 and 1 from the vertex buffer compose the 
		// first triangle of this mesh.
		std::vector<unsigned short> earthIndices ((numSegments - 1) * (numSegments - 1) * 6);
		
		for (UINT i = 0; i < numSegments - 1; i++)
			for (UINT j = 0; j < numSegments - 1; j++)
			{
				int index = (i + j * (numSegments - 1)) * 6;

				// tri 1
				earthIndices[index + 0] = (i + 0 + (j + 0) * numSegments);
				earthIndices[index + 1] = (i + 1 + (j + 0) * numSegments);
				earthIndices[index + 2] = (i + 0 + (j + 1) * numSegments);

				// tri 2
				earthIndices[index + 3] = (i + 0 + (j + 1) * numSegments);
				earthIndices[index + 4] = (i + 1 + (j + 0) * numSegments);
				earthIndices[index + 5] = (i + 1 + (j + 1) * numSegments);
			}

		D3D11_SUBRESOURCE_DATA vertexBufferData = {0};
		vertexBufferData.pSysMem = &earthVertices[0];
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(VertexPositionNormalColor) * earthVertices.size(), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&vertexBufferDesc,
				&vertexBufferData,
				&m_vertexBuffer
				)
			);

		m_indexCount = earthIndices.size();

		D3D11_SUBRESOURCE_DATA indexBufferData = {0};
		indexBufferData.pSysMem = &earthIndices[0];
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(unsigned short) * m_indexCount, D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&indexBufferDesc,
				&indexBufferData,
				&m_indexBuffer
				)
			);
	});
	
	// Once the cube is loaded, the object is ready to be rendered.
	createEarthTask.then([this] () {
		m_loadingComplete = true;
	});
}

void Sample3DSceneRenderer::ReleaseDeviceDependentResources()
{
	m_loadingComplete = false;
	m_vertexShader.Reset();
	m_vertexShaderEarth.Reset();
	m_inputLayout.Reset();
	m_pixelShader.Reset();
	m_pixelShaderEarth.Reset();

	for (uint32 i = 0; i < PyramidNumViews; i++)
	{
		m_constantBufferEarth[i].Reset();
		m_constantBufferCloud[i].Reset();
		m_constantBufferStars[i].Reset();
	}

	m_constantBufferLightSources.Reset();
	m_vertexBuffer.Reset();
	m_indexBuffer.Reset();
}