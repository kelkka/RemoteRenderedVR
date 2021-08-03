#include "D3DCompressor.h"
#include <stdio.h>
#include <cuda_gl_interop.h> //includes GL.h
#include "RenderHelper.h"
#include "StatCalc.h"


D3DCompressor::D3DCompressor()
{

}

D3DCompressor::~D3DCompressor()
{
	if(m_compressionTexture[0] != nullptr)
		m_compressionTexture[0]->Release();
	if (m_compressionTexture[1] != nullptr)
		m_compressionTexture[1]->Release();
	if(m_samplerState != nullptr)
		m_samplerState->Release();
	if (m_cbExtra != nullptr)
		m_cbExtra->Release();
	if (m_depthStencilState != nullptr)
		m_depthStencilState->Release();
	if(m_shader != nullptr)
		delete m_shader;
}

HRESULT D3DCompressor::LoadMaps(ID3D11Device* _device, ID3D11DeviceContext* _deviceContext)
{
	m_device = _device;
	m_deviceContext = _deviceContext;

	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

	HR_CHECK(m_device->CreateSamplerState(&sampDesc, &m_samplerState))

	std::string path = StatCalc::GetParameterStr("LENS_MAPPING_PATH");

	for(int i=0;i<2;i++)
		LoadTexture_Internal(path, i);

	return 0;
}

HRESULT D3DCompressor::LoadTexture_Internal(std::string path, int _eye)
{
	if (_eye == 0)
		path += "_left.map";
	else
		path += "_right.map";

	FILE* input;

	if (fopen_s(&input, path.c_str(), "rb") != 0)
	{
		printf("Failed to read encoder map %s\n", path.c_str());
		return -1;
	}

	HR_CHECK(MakeTexture(_eye, input))

	fclose(input);

	return 0;
}

HRESULT D3DCompressor::MakeTexture(int _eye, FILE* _file)
{
	//The encoded size, will be overwritten by the second eye but should be the same anyway
	fread(&m_encodedSize, sizeof(glm::ivec2), 1, _file);

	//The decoded size
	fread(&m_decodedSize, sizeof(glm::ivec2), 1, _file);
	Pixel* map = (Pixel*)malloc(m_decodedSize.x * m_decodedSize.y * sizeof(Pixel));

	//The data
	fread(map, sizeof(Pixel), m_decodedSize.x * m_decodedSize.y, _file);

	printf("Read %d,%d compression map\n", m_decodedSize.x, m_decodedSize.y);

	D3D11_TEXTURE2D_DESC desc;
	desc.Width = m_decodedSize.x;
	desc.Height = m_decodedSize.y;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R16G16_UINT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = map;
	initData.SysMemPitch = m_decodedSize.x * 4;
	initData.SysMemSlicePitch = 0;

	ID3D11Texture2D* texture2D = nullptr;
	HR_CHECK(m_device->CreateTexture2D(&desc, &initData, &texture2D))

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	memset(&SRVDesc, 0, sizeof(SRVDesc));
	SRVDesc.Format = DXGI_FORMAT_R16G16_UINT;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MipLevels = 1;

	HR_CHECK(m_device->CreateShaderResourceView(texture2D, &SRVDesc, &m_compressionTexture[_eye]))

	texture2D->Release();

	CreateFrameBuffer_Internal(m_frameBuffer[_eye], m_encodedSize.x, m_encodedSize.y, m_device);

	LoadShader();
	CreateQuad(1);

	return 0;
}

const glm::ivec2& D3DCompressor::GetCompressedSize()
{
	return m_encodedSize;
}

HRESULT D3DCompressor::LoadShader()
{
	if (m_shader != nullptr)
		delete m_shader;

	m_shader = new D3DShader();

	ID3DBlob* shaderBlob;

	//VERTEX
	{
		HR_CHECK(D3DShader::CompileShader(L"../content/shaders/D3D/VS_Compressor.hlsl", "vs_5_0", &shaderBlob))

			D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		HR_CHECK(m_device->CreateInputLayout(vertexDesc, 1, shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), &m_shader->InputLayout))
		HR_CHECK(m_device->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, &m_shader->VS))
		shaderBlob->Release();
	}
	//PIXEL
	{
		HR_CHECK(D3DShader::CompileShader(L"../content/shaders/D3D/PS_Compressor.hlsl", "ps_5_0", &shaderBlob))
		HR_CHECK(m_device->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), NULL, &m_shader->PS))
		shaderBlob->Release();
	}

	//SET constant buffer
	struct ExtraData
	{
		int gHeight = 0;
		int paddding0 = 0;
		int paddding1 = 0;
		int paddding2 = 0;
	};
	ExtraData extra;
	extra.gHeight = m_decodedSize.y;
	CreateConstantBuffer(&extra, sizeof(ExtraData), &m_cbExtra, m_device, D3D11_USAGE_IMMUTABLE);

	//SET Stencil state
	D3D11_DEPTH_STENCIL_DESC stencilDesc;

	stencilDesc.DepthEnable = false;
	stencilDesc.StencilEnable = false;

	//Just to fill default values
	stencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	stencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
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

	m_device->CreateDepthStencilState(&stencilDesc, &m_depthStencilState);

	return 0;
}

void D3DCompressor::Render(const FrameBufferInfo& _frameBuffer, int _eye)
{
	m_deviceContext->IASetInputLayout(m_shader->InputLayout);
	m_deviceContext->VSSetShader(m_shader->VS, NULL, 0);
	m_deviceContext->PSSetShader(m_shader->PS, NULL, 0);

	m_deviceContext->PSSetConstantBuffers(0, 1, &m_cbExtra);

	m_deviceContext->OMSetDepthStencilState(m_depthStencilState, 0);

	m_deviceContext->ClearRenderTargetView(m_frameBuffer[_eye].RenderTargetView, CLEAR_COLOR);

	m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_deviceContext->IASetVertexBuffers(0, 1, &m_quad.VertexBuffer, &m_quad.Stride, &m_quad.Offset);

	SetViewport(m_frameBuffer[_eye], m_frameBuffer[_eye].Width, m_frameBuffer[_eye].Height, m_deviceContext);

	m_deviceContext->PSSetShaderResources(0, 1, &_frameBuffer.ShaderResourceView);
	m_deviceContext->PSSetShaderResources(1, 1, &m_compressionTexture[_eye]);

	m_deviceContext->Draw(m_quad.NumVertices, 0);

	ID3D11ShaderResourceView* nullSRV = nullptr;

	m_deviceContext->PSSetShaderResources(0, 1, &nullSRV);
	m_deviceContext->PSSetShaderResources(1, 0, &nullSRV);
}

ID3D11Texture2D* D3DCompressor::GetEncodedTexture(int _eye)
{
	return m_frameBuffer[_eye].FrameTexture;
}

const FrameBufferInfo& D3DCompressor::GetFrameBuffer(int _eye)
{
	return m_frameBuffer[_eye];
}

void D3DCompressor::CreateQuad(float t)
{
	Vertex modelQuad[6];

	modelQuad[0].Pos.x = t;
	modelQuad[0].Pos.y = -t;
	//modelQuad[0].Pos.z = 0;
	//modelQuad[0].Normal.x = 0;
	//modelQuad[0].Normal.y = 0;
	//modelQuad[0].Normal.z = -1;
	//modelQuad[0].TexCoord.x = 1;
	//modelQuad[0].TexCoord.y = 0;

	modelQuad[1].Pos.x = -t;
	modelQuad[1].Pos.y = -t;
	//modelQuad[1].Pos.z = 0;
	//modelQuad[1].Normal.x = 0;
	//modelQuad[1].Normal.y = 0;
	//modelQuad[1].Normal.z = -1;
	//modelQuad[1].TexCoord.x = 0;
	//modelQuad[1].TexCoord.y = 0;

	modelQuad[2].Pos.x = -t;
	modelQuad[2].Pos.y = t;
	//modelQuad[2].Pos.z = 0;
	//modelQuad[2].Normal.x = 0;
	//modelQuad[2].Normal.y = 0;
	//modelQuad[2].Normal.z = -1;
	//modelQuad[2].TexCoord.x = 0;
	//modelQuad[2].TexCoord.y = 1;

	modelQuad[3].Pos.x = t;
	modelQuad[3].Pos.y = -t;
	//modelQuad[3].Pos.z = 0;
	//modelQuad[3].Normal.x = 0;
	//modelQuad[3].Normal.y = 0;
	//modelQuad[3].Normal.z = -1;
	//modelQuad[3].TexCoord.x = 1;
	//modelQuad[3].TexCoord.y = 0;

	modelQuad[4].Pos.x = -t;
	modelQuad[4].Pos.y = t;
	//modelQuad[4].Pos.z = 0;
	//modelQuad[4].Normal.x = 0;
	//modelQuad[4].Normal.y = 0;
	//modelQuad[4].Normal.z = -1;
	//modelQuad[4].TexCoord.x = 0;
	//modelQuad[4].TexCoord.y = 1;

	modelQuad[5].Pos.x = t;
	modelQuad[5].Pos.y = t;
	//modelQuad[5].Pos.z = 0;
	//modelQuad[5].Normal.x = 0;
	//modelQuad[5].Normal.y = 0;
	//modelQuad[5].Normal.z = -1;
	//modelQuad[5].TexCoord.x = 1;
	//modelQuad[5].TexCoord.y = 1;

	CreateVertexBuffer_Internal(modelQuad, m_quad, 3, 6, m_device);
}