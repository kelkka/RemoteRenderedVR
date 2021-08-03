#include "D3DRenderer.h"
#include <stdio.h>
#include <d3dcompiler.h>
#include <SDL_syswm.h>

#include "glm/common.hpp"
#include "PNG.h"
#include <iostream>
#include <dxgi1_3.h>

#include <d3d9helper.h>

#pragma comment( lib, "d3d11" )
#pragma comment( lib, "d3dcompiler.lib" ) // shader compiler

D3DRenderer::~D3DRenderer()
{
	DeleteAllShaders();
	//m_swapChain->Release();
	m_deviceContext->Release();
	m_deferredContext->Release();
	m_rasterizerStateBackCull->Release();
	m_rasterizerStateFrontCull->Release();
	m_rasterizerStateNoCull->Release();
	m_rasterizerStateCCW->Release();
	m_cbModelBuffer->Release();
	m_cbLightBuffer->Release();
	m_samplerState->Release();
#ifdef LOCAL_TESTING_ONLY
	m_backBuffer->Release();
	m_swapChain->Release();
#endif
	m_depthStencilState->Release();
	//m_deferredContext->Release();
	m_depthStencilStateInitialize->Release();

	for (int i = 0; i < NUM_QUERIES; i++)
	{
		m_disjointQueries[i]->Release();

		for (int j = 0; j < 2; j++)
		{
			m_timerQueries[i][j]->Release();
		}
	}

	ID3D11Debug* debug;
	m_device->QueryInterface(&debug);
	debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);

	m_device->Release();
	debug->Release();



	printf("Exiting D3DRenderer\n");
}

void D3DRenderer::DrawCubesBegin(const FrameBufferInfo& _eye, const D3DModel& _model, ID3D11DeviceContext* _deviceContext)
{
	D3DShader* shader = m_shaders[0];

	_deviceContext->IASetInputLayout(shader->InputLayout);
	_deviceContext->VSSetShader(shader->VS, NULL, 0);
	_deviceContext->PSSetShader(shader->PS, NULL, 0);

	m_cbLight.LightColor = glm::vec3(1, 1, 1);
	m_cbLight.LightPos = glm::vec3(0, 2, 0);
	m_cbLight.IsWorldMesh = (int)StatCalc::GetParameter("USE_WORLD_MESH");

	D3D11_MAPPED_SUBRESOURCE mappedBuffer;
	_deviceContext->Map(m_cbLightBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedBuffer);
	memcpy(mappedBuffer.pData, &m_cbLight, sizeof(CBlight));
	_deviceContext->Unmap(m_cbLightBuffer, 0);

	_deviceContext->VSSetConstantBuffers(0, 1, &m_cbModelBuffer);
	_deviceContext->PSSetConstantBuffers(0, 1, &m_cbLightBuffer);
	_deviceContext->PSSetConstantBuffers(1, 1, &m_cbModelBuffer);

	_deviceContext->OMSetDepthStencilState(m_depthStencilState, 0);

	_deviceContext->OMSetRenderTargets(1, &_eye.RenderTargetView, _eye.DepthStencilView);
	_deviceContext->ClearRenderTargetView(_eye.RenderTargetView, CLEAR_COLOR);
	_deviceContext->ClearDepthStencilView(_eye.DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_deviceContext->IASetVertexBuffers(0, 1, &_model.VertexBuffer, &_model.Stride, &_model.Offset);

	SetViewport(_eye, _eye.Width, _eye.Height, _deviceContext);
}

void D3DRenderer::Present()
{
#ifdef LOCAL_TESTING_ONLY
	m_swapChain->Present(0, 0);
#endif
}

void D3DRenderer::SetCullFront(bool _cullFront, ID3D11DeviceContext* _deviceContext)
{
	if (_cullFront)
	{
		_deviceContext->RSSetState(m_rasterizerStateFrontCull);
	}
	else
	{
		_deviceContext->RSSetState(m_rasterizerStateBackCull);
	}
}

void D3DRenderer::SetNoCull(ID3D11DeviceContext* _deviceContext)
{
	_deviceContext->RSSetState(m_rasterizerStateNoCull);
}

void D3DRenderer::DrawCube(vr::Hmd_Eye _eye, float _normalDir, const glm::vec3& _pos, const glm::vec3& _scale, ID3D11ShaderResourceView* _textureShaderResourceView, ID3D11DeviceContext* _deviceContext, const D3DModel& _model, float _repeat)
{
	//if (_normalDir < 0)
	//{
	//	_deviceContext->RSSetState(m_rasterizerStateFrontCull);
	//}
	//else
	//{
	//	_deviceContext->RSSetState(m_rasterizerStateBackCull);
	//}

	//TODO:
	SetNoCull(_deviceContext);

	m_cbModel.NormalDirection = _normalDir;
	m_cbModel.M = glm::mat4(1);
	m_cbModel.M = glm::translate(m_cbModel.M, _pos);
	m_cbModel.M = glm::scale(m_cbModel.M, glm::vec3(_scale.x, _scale.y, _scale.z));
	m_cbModel.V = m_VRInputs->GetV(_eye);
	m_cbModel.P = m_VRInputs->GetP(_eye);
	m_cbModel.RepeatTexture = _repeat;

	glm::mat4 HMDworld = glm::mat4(1);
	m_VRInputs->GetHMDWorld(HMDworld);
	m_cbModel.EyePos = glm::vec3(HMDworld[3][0], HMDworld[3][1], HMDworld[3][2]);

	D3D11_MAPPED_SUBRESOURCE mappedBuffer;
	_deviceContext->Map(m_cbModelBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedBuffer);
	memcpy(mappedBuffer.pData, &m_cbModel, sizeof(CBmodel));
	_deviceContext->Unmap(m_cbModelBuffer, 0);
	
	_deviceContext->PSSetShaderResources(0, 1, &_textureShaderResourceView);
	_deviceContext->PSSetSamplers(0, 1, &m_samplerState);
	_deviceContext->Draw(_model.NumVertices, 0);
}

void D3DRenderer::EndDraw()
{
	ID3D11CommandList* commandList;
	m_deferredContext->FinishCommandList(FALSE, &commandList);
	m_deviceContext->ExecuteCommandList(commandList, FALSE);
	commandList->Release();

	//while (S_FALSE == pEvent->GetData(NULL, 0, D3DGETDATA_FLUSH))
	//	;
}

void D3DRenderer::DrawCompanionWindow()
{

}

const FrameBufferInfo& D3DRenderer::GetCompanionWindow()
{
	return m_companionFb;
}

HRESULT D3DRenderer::InitD3D(int _width, int _height)
{
	m_width = _width;
	m_height = _height;

	m_companionFb.Width = m_width;
	m_companionFb.Height = m_height;

	HR_CHECK(CreateDevice())

#ifdef LOCAL_TESTING_ONLY
		HR_CHECK(m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&m_backBuffer)))
		HR_CHECK(CreateRendertarget(m_backBuffer, &m_companionFb.RenderTargetView, m_device))
		HR_CHECK(CreateDepthStencil(m_companionFb, m_device))
#endif
		HR_CHECK(CreateShaders())

		HR_CHECK(CreateRasterizerState(m_deviceContext))

		//HR_CHECK(CreateConstantBuffer(&m_cbCamera, sizeof(CBcamera), &m_cbCameraBuffer))
		HR_CHECK(CreateConstantBuffer(&m_cbLight, sizeof(CBlight), &m_cbLightBuffer, m_device))
		HR_CHECK(CreateConstantBuffer(&m_cbModel, sizeof(CBmodel), &m_cbModelBuffer, m_device))

		HR_CHECK(CreateDepthStencilStates())

	//Queries
	D3D11_QUERY_DESC desc;
	desc.MiscFlags = 0;

	for (int i = 0; i < NUM_QUERIES; i++)
	{
		desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
		HR_CHECK(m_device->CreateQuery(&desc, &m_disjointQueries[i]);)

			for (int k = 0; k < 2; k++)
			{
				desc.Query = D3D11_QUERY_TIMESTAMP;
				HR_CHECK(m_device->CreateQuery(&desc, &m_timerQueries[i][k]);)
			}
	}


	return 0;
}

void D3DRenderer::TimerStart()
{
	m_deviceContext->Begin(m_disjointQueries[m_currentQuery]);
	m_deviceContext->End(m_timerQueries[m_currentQuery][0]);
}

long long D3DRenderer::TimerEnd()
{
	//static int ticks = 0;

	//ticks++;

	//if (ticks <= 1)
	//	return;

	m_deviceContext->End(m_disjointQueries[m_currentQuery]);
	m_deviceContext->End(m_timerQueries[m_currentQuery][1]);

	D3D11_QUERY_DATA_TIMESTAMP_DISJOINT stamp;
	while (S_OK != m_deviceContext->GetData(m_disjointQueries[m_currentQuery], &stamp, sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT), 0))
	{
		//Busy wait
		//printf("waiting\n");
	}

	//m_currentQuery = (m_currentQuery + 1) % NUM_QUERIES;

	if (stamp.Disjoint)
	{
		//bad
		printf("Timestamps disjoint\n");
		return 0;
	}

	UINT64 t0;
	if (S_OK != m_deviceContext->GetData(m_timerQueries[m_currentQuery][0], &t0, sizeof(UINT64), 0))
	{
		printf("Failed to get timestamp 0\n");
		return 0;
	}

	UINT64 t1;
	if (S_OK != m_deviceContext->GetData(m_timerQueries[m_currentQuery][1], &t1, sizeof(UINT64), 0))
	{
		printf("Failed to get timestamp 0\n");
		return 0;
	}

	double consumed = double(t1 - t0) / double(stamp.Frequency);
	
	//printf("Consumed %fs\n", consumed);

	return long long(consumed * 1000000);
	
}

ID3D11Device*& D3DRenderer::GetDevice()
{
	return m_device;
}

ID3D11DeviceContext*& D3DRenderer::GetDeviceContext()
{
	return m_deviceContext;
}

HRESULT D3DRenderer::LoadTexture(const char* _path, ID3D11ShaderResourceView** _shaderRscView, D3D11_TEXTURE_ADDRESS_MODE _addressMode)
{
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_POINT;
	sampDesc.AddressU = _addressMode;
	sampDesc.AddressV = _addressMode;
	sampDesc.AddressW = _addressMode;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

	HR_CHECK(m_device->CreateSamplerState(&sampDesc, &m_samplerState))

	int imgWidth = 0;
	int imgHeight = 0;
	int channels = 0;

	unsigned char* imgData = PNG::ReadPNG(_path, imgWidth, imgHeight, channels, 4);

	if (imgData == NULL)
	{
		printf("Failed to load image\n");
		return -1;
	}

	D3D11_TEXTURE2D_DESC desc;
	desc.Width = imgWidth;
	desc.Height = imgHeight;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = imgData;
	initData.SysMemPitch = imgWidth * 4;
	initData.SysMemSlicePitch = 0;

	ID3D11Texture2D* texture2D = nullptr;
	HR_CHECK(m_device->CreateTexture2D(&desc, &initData, &texture2D))

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	memset(&SRVDesc, 0, sizeof(SRVDesc));
	SRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MipLevels = 1;

	HR_CHECK(m_device->CreateShaderResourceView(texture2D, &SRVDesc, _shaderRscView))
	texture2D->Release();
	free(imgData);

	return 0;
}

HRESULT D3DRenderer::CreateRasterizerState(ID3D11DeviceContext* _deviceContext)
{
	D3D11_RASTERIZER_DESC rasterizerDesc; //Default

	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	rasterizerDesc.FrontCounterClockwise = FALSE;
	rasterizerDesc.DepthBias = D3D11_DEFAULT_DEPTH_BIAS;
	rasterizerDesc.DepthBiasClamp = D3D11_DEFAULT_DEPTH_BIAS_CLAMP;
	rasterizerDesc.SlopeScaledDepthBias = D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.ScissorEnable = FALSE;
	rasterizerDesc.MultisampleEnable = FALSE;
	rasterizerDesc.AntialiasedLineEnable = FALSE;

	HR_CHECK(m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerStateBackCull))

	rasterizerDesc.CullMode = D3D11_CULL_FRONT;

	HR_CHECK(m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerStateFrontCull))

	rasterizerDesc.CullMode = D3D11_CULL_NONE;

	HR_CHECK(m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerStateNoCull))

	rasterizerDesc.FrontCounterClockwise = TRUE;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;

	HR_CHECK(m_device->CreateRasterizerState(&rasterizerDesc, &m_rasterizerStateCCW))

	_deviceContext->RSSetState(m_rasterizerStateBackCull);

	return 0;
}

void D3DRenderer::DeleteAllShaders()
{
	for (int i = 0; i < m_shaders.size(); i++)
	{
		delete m_shaders[i];
	}
	m_shaders.clear();
}

HRESULT D3DRenderer::CreateShaders()
{
	DeleteAllShaders();
	ID3DBlob* shaderBlob;

	//VERTEX
	{
		m_shaders.push_back(new D3DShader());
		D3DShader* shader = m_shaders.back();

		HR_CHECK(D3DShader::CompileShader(L"../content/shaders/D3D/VS_Cube.hlsl", "vs_5_0", &shaderBlob))

		D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0 },
			{"TEX_COORD", 0, DXGI_FORMAT_R32G32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0 },
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,0,20,D3D11_INPUT_PER_VERTEX_DATA,0 }
		};

		HR_CHECK(m_device->CreateInputLayout(vertexDesc, 3, shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), &shader->InputLayout))
		HR_CHECK(m_device->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, &shader->VS))
		shaderBlob->Release();
	}
	//PIXEL
	{
		D3DShader* shader = m_shaders.back();
		HR_CHECK(D3DShader::CompileShader(L"../content/shaders/D3D/PS_Cube.hlsl", "ps_5_0", &shaderBlob))
		HR_CHECK(m_device->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, &shader->PS))
		shaderBlob->Release();
	}


	//VERTEX
	{
		m_shaders.push_back(new D3DShader());
		D3DShader* shader = m_shaders.back();

		HR_CHECK(D3DShader::CompileShader(L"../content/shaders/D3D/VS_Stencil.hlsl", "vs_5_0", &shaderBlob))
		D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA,0 },
		};

		HR_CHECK(m_device->CreateInputLayout(vertexDesc, 1, shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), &shader->InputLayout))
		HR_CHECK(m_device->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, &shader->VS))
		shaderBlob->Release();
	}
	//PIXEL
	{
		D3DShader* shader = m_shaders.back();
		HR_CHECK(D3DShader::CompileShader(L"../content/shaders/D3D/PS_Stencil.hlsl", "ps_5_0", &shaderBlob))
		HR_CHECK(m_device->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, &shader->PS))
		shaderBlob->Release();
	}

	printf("CreateShaders() OK\n");

	return 0;
}

HRESULT D3DRenderer::CreateVertexBuffer(void* _data, D3DModel& _model, UINT _elementSize, UINT _elements)
{
	return CreateVertexBuffer_Internal(_data, _model, _elementSize, _elements, m_device);
}

HRESULT D3DRenderer::CreateFrameBuffer(FrameBufferInfo& _eye, int _w, int _h)
{
	HR_CHECK(CreateFrameBuffer_Internal(_eye, _w, _h, m_device))

	return 0;
}

HRESULT D3DRenderer::CreateTexture2D(const D3D11_TEXTURE2D_DESC* _desc, ID3D11Texture2D** _tex)
{
	HR_CHECK(m_device->CreateTexture2D(_desc, 0, _tex))

	return 0;
}

HRESULT D3DRenderer::CopyTexture(ID3D11Texture2D* _dest, ID3D11Texture2D* _src, unsigned char* _outData, int _w, int _h, int _channels)
{
	m_deviceContext->CopyResource(_dest, _src);

	D3D11_MAPPED_SUBRESOURCE mappedBuffer;
	m_deviceContext->Map(_dest, 0, D3D11_MAP_READ, 0, &mappedBuffer);

	for (int y = 0; y < _h; y++)
	{
		memcpy(_outData + _w * _channels * y, (unsigned char*)mappedBuffer.pData + mappedBuffer.RowPitch * y, mappedBuffer.RowPitch);
	}

	//Swap R and B back because they have to be BGRA in the encoder for some reason
	for (int y = 0; y < _h; y++)
	{
		for (int x = 0; x < _w; x++)
		{
			int B = (x + y * _w) * _channels;
			int G = B + 1;
			int R = G + 1;

			unsigned char colorB = _outData[B];
			unsigned char colorR = _outData[R];

			_outData[B] = colorR;
			_outData[R] = colorB;
		}
	}
	
	m_deviceContext->Unmap(_dest, 0);

	return 0;
}

ID3D11DeviceContext* D3DRenderer::GetDeferredDeviceContext()
{
	return m_deferredContext;
}

//Render stencil once
void D3DRenderer::RenderDepthStencil(FrameBufferInfo& _frameBuffer, int _eyeIndex)
{
	const HiddenAreaMesh& stencilMesh = m_VRInputs->GetHiddenAreaMesh(_eyeIndex);

	D3DModel mesh;

	CreateVertexBuffer(stencilMesh.VertexData, mesh, 2, stencilMesh.TriangleCount * 3);

	SetViewport(_frameBuffer, _frameBuffer.Width, _frameBuffer.Height, m_deviceContext);

	if(_eyeIndex == 0)
		m_deviceContext->RSSetState(m_rasterizerStateBackCull);
	else
		m_deviceContext->RSSetState(m_rasterizerStateCCW);

	m_deviceContext->ClearRenderTargetView(_frameBuffer.RenderTargetView, CLEAR_COLOR);
	m_deviceContext->ClearDepthStencilView(_frameBuffer.DepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0.0f, 0);

	m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_deviceContext->IASetVertexBuffers(0, 1, &mesh.VertexBuffer, &mesh.Stride, &mesh.Offset);
	m_deviceContext->OMSetDepthStencilState(m_depthStencilStateInitialize, 1);

	D3DShader* shader = m_shaders[1];

	m_deviceContext->IASetInputLayout(shader->InputLayout);
	m_deviceContext->VSSetShader(shader->VS, NULL, 0);
	m_deviceContext->PSSetShader(shader->PS, NULL, 0);

	m_deviceContext->Draw(mesh.NumVertices, 0);

	//m_shaders[3]->UseProgram();

	//glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // Do not draw any pixels on the back buffer
	//glEnable(GL_STENCIL_TEST); // Enables testing AND writing functionalities
	//glStencilFunc(GL_EQUAL, 1, 0xFF); // Do not test the current value in the stencil buffer, always accept any value on there for drawing
	//glStencilMask(0xFF);
	//glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE); // Make every test succeed

	////Both eyes are the same tri count
	//const HiddenAreaMesh& stencilMesh = m_VRInputs->GetHiddenAreaMesh(_eye);

	//glBindVertexArray(m_stencilVAO[_eye]);
	//glDrawArrays(GL_TRIANGLES, 0, stencilMesh.TriangleCount * 3);

	//glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP); // Make sure you will no longer (over)write stencil values, even if any test succeeds
	//glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); // Make sure we draw on the backbuffer again.
	//glStencilFunc(GL_EQUAL, 0, 0xFF); // Now we will only draw pixels where the corresponding stencil buffer value equals 1

	//glBindVertexArray(0);

	//GLErrorCheck(__func__, __LINE__);
}

HRESULT D3DRenderer::CreateDepthStencilStates()
{
	D3D11_DEPTH_STENCIL_DESC stencilDesc;

	stencilDesc.DepthEnable = true;
	stencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	stencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	stencilDesc.StencilEnable = true;
	stencilDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	stencilDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

	stencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	stencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	stencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	stencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;

	stencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	stencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	stencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	stencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;

	HR_CHECK(m_device->CreateDepthStencilState(&stencilDesc, &m_depthStencilState))

	stencilDesc.DepthEnable = false;
	stencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	stencilDesc.DepthFunc = D3D11_COMPARISON_NEVER;
	stencilDesc.StencilEnable = true;
	stencilDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	stencilDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

	stencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	stencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	stencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	stencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	HR_CHECK(m_device->CreateDepthStencilState(&stencilDesc, &m_depthStencilStateInitialize))

	return 0;
}

HRESULT D3DRenderer::CreateDevice()
{
	//Use DXGI to get a factory to set swap chain
	//IDXGIDevice* dxgiDevice = 0;
	IDXGIAdapter* dxgiAdapter = 0;
	IDXGIFactory2* dxgiFactory = 0;

	//HR_CHECK(m_device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice))
	//HR_CHECK(dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter))
	//HR_CHECK(dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), (void**)&dxgiFactory))

	HR_CHECK(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG ,__uuidof(IDXGIFactory2), (void**)&dxgiFactory));

	int bestIndex = 0;

	for (UINT i = 0;
		dxgiFactory->EnumAdapters(i, &dxgiAdapter) != DXGI_ERROR_NOT_FOUND;
		i++)
	{
		DXGI_ADAPTER_DESC adapterDesc;
		dxgiAdapter->GetDesc(&adapterDesc);
		char szDesc[80];
		wcstombs(szDesc, adapterDesc.Description, sizeof(szDesc));

		std::string adapterName = szDesc;

		printf("GPU (%d) %s", i, szDesc);

		if (adapterName.find("NVIDIA") != std::string::npos)
		{
			bestIndex = i;
			printf("	WILL USE THIS\n");
			continue;
		}
	}

	dxgiFactory->EnumAdapters(bestIndex, &dxgiAdapter);


	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;

	//Create device
	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
	swapChainDesc.Width = m_width;
	swapChainDesc.Height = m_height;
	swapChainDesc.Format = format;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.Flags = 0;
	
	HR_CHECK(D3D11CreateDevice(
		dxgiAdapter,				//Default display
		D3D_DRIVER_TYPE_UNKNOWN,	//Driver
		NULL,						//Optional software driver
		D3D11_CREATE_DEVICE_DEBUG,	//Flags, debug info
		NULL,				//Feature level, choose newest
		0,							//Number of feature levels
		D3D11_SDK_VERSION,			//SDK Version macro
		&m_device,					//The created device
		&featureLevel,				//The used feature level
		&m_deviceContext			//The created device context
	))


	//m_deferredContext = m_deviceContext

	m_device->CreateDeferredContext(
			0,
			&m_deferredContext
		);

	printf("Using D3D %d\n", featureLevel);

	SetProcessDPIAware();
#ifdef LOCAL_TESTING_ONLY
	Uint32 unWindowFlags = SDL_WINDOW_SHOWN;
	const char* name = "Server";
	m_companionWindowHeight = 1280;
	m_companionWindowWidth = uint32_t(m_companionWindowHeight * 0.9f);
	m_pCompanionWindow = SDL_CreateWindow(name, 0, 0, m_companionWindowWidth, m_companionWindowHeight, unWindowFlags);

	SDL_SysWMinfo windowinfo;
	SDL_VERSION(&windowinfo.version);
	SDL_GetWindowWMInfo(m_pCompanionWindow, &windowinfo);

	//Make swap chain
	HR_CHECK(dxgiFactory->CreateSwapChainForHwnd(
			m_device,
			windowinfo.info.win.window,
			&swapChainDesc,
			NULL,
			NULL,
			&m_swapChain))

#endif

	//DXGI_SWAP_CHAIN_DESC swapChainDesc;
	//swapChainDesc.Width = m_width;
	//swapChainDesc.Height = m_height;
	//swapChainDesc.Format = format;
	//swapChainDesc.Stereo = FALSE;
	//swapChainDesc.SampleDesc.Count = 1;
	//swapChainDesc.SampleDesc.Quality = 0;
	//swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	//swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	//swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	//swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	//swapChainDesc.BufferCount = 2;
	//swapChainDesc.Flags = 0;

	//HR_CHECK(CreateSwapChain(
	//	m_device,
	//	&swapChainDesc,
	//	&m_swapChain))

	//	CreateSwapChain(,)

	//Release COM stuff
	//dxgiDevice->Release();
	dxgiAdapter->Release();
	dxgiFactory->Release();

	return 0;
}
