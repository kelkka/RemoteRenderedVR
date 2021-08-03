#include "D3DHelper.h"

HRESULT CreateFrameBuffer_Internal(FrameBufferInfo& _eye, int _w, int _h, ID3D11Device*& _device)
{
	_eye.Width = _w;
	_eye.Height = _h;

	D3D11_TEXTURE2D_DESC texDesc;

	texDesc.Width = _w;
	texDesc.Height = _h;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	HR_CHECK(_device->CreateTexture2D(&texDesc, 0, &_eye.FrameTexture))
	HR_CHECK(CreateRendertarget(_eye.FrameTexture, &_eye.RenderTargetView, _device))
	HR_CHECK(CreateDepthStencil(_eye, _device))

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	memset(&SRVDesc, 0, sizeof(SRVDesc));
	SRVDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MipLevels = 1;

	HR_CHECK(_device->CreateShaderResourceView(_eye.FrameTexture, &SRVDesc, &_eye.ShaderResourceView))

	return 0;
}

HRESULT CreateRendertarget(ID3D11Resource* _resource, ID3D11RenderTargetView** _renderTargetView, ID3D11Device*& _device)
{
	HR_CHECK(_device->CreateRenderTargetView(_resource, NULL, _renderTargetView))

		return 0;
}

HRESULT CreateDepthStencil(FrameBufferInfo& _eye, ID3D11Device*& _device)
{
	D3D11_TEXTURE2D_DESC textureDesc;

	textureDesc.Width = _eye.Width;
	textureDesc.Height = _eye.Height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;

	HR_CHECK(_device->CreateTexture2D(&textureDesc, 0, &_eye.DepthStencilBuffer))
		HR_CHECK(_device->CreateDepthStencilView(_eye.DepthStencilBuffer, 0, &_eye.DepthStencilView))

		return 0;
}

HRESULT CreateVertexBuffer_Internal(void* _data, D3DModel& _model, UINT _vertexElements, UINT _vertices, ID3D11Device*& _device)
{
	D3D11_BUFFER_DESC bufferDesc;
	D3D11_SUBRESOURCE_DATA subData;

	bufferDesc.ByteWidth = _vertexElements * _vertices * sizeof(float);
	bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	subData.pSysMem = _data;
	subData.SysMemPitch = 0;
	subData.SysMemSlicePitch = 0;

	HR_CHECK(_device->CreateBuffer(&bufferDesc, &subData, &_model.VertexBuffer))

	_model.Stride = _vertexElements * sizeof(float);
	_model.NumVertices = _vertices;

	return 0;
}

HRESULT CreateConstantBuffer(void* _memory, UINT _size, ID3D11Buffer** _buffer, ID3D11Device* _device, D3D11_USAGE _usage)
{
	D3D11_BUFFER_DESC bufferDesc;
	D3D11_SUBRESOURCE_DATA subData;

	bufferDesc.ByteWidth = _size;
	bufferDesc.Usage = _usage;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = _usage == D3D11_USAGE_IMMUTABLE ? 0 : D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	subData.pSysMem = _memory;
	subData.SysMemPitch = 0;
	subData.SysMemSlicePitch = 0;

	HR_CHECK(_device->CreateBuffer(&bufferDesc, &subData, _buffer))

		return 0;
}

void SetViewport(const FrameBufferInfo& _eye, UINT _width, UINT _height, ID3D11DeviceContext*& _deviceContext)
{
	_deviceContext->OMSetRenderTargets(1, &_eye.RenderTargetView, _eye.DepthStencilView);

	D3D11_VIEWPORT viewPort;

	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;
	viewPort.Width = (FLOAT)_width;
	viewPort.Height = (FLOAT)_height;
	viewPort.MinDepth = 0;
	viewPort.MaxDepth = 1;

	_deviceContext->RSSetViewports(1, &viewPort);
}

void ReportError(HRESULT hr, int _line, const char* _function)
{
	_com_error err(hr); 
	LPCTSTR errMsg = err.ErrorMessage(); 
	printf("Error: %ld Line %d %s\n%ws\n", hr, _line, _function, errMsg);
	system("pause"); 
}
