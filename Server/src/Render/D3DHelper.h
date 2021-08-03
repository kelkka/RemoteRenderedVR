#pragma once

#include <glm.hpp>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <comdef.h>
#include <wtypes.h>

const FLOAT CLEAR_COLOR[4] = { 0.0f,0.0f,0.0f,1.0f };

struct Vertex
{
	glm::vec3 Pos;
	//glm::vec3 Normal;
	//glm::vec2 TexCoord;
};

struct CBmodel
{
	glm::mat4 P;
	glm::mat4 V;
	glm::mat4 M;
	glm::vec3 EyePos;
	float NormalDirection;
	float RepeatTexture;
	glm::vec3 pad0;
};

struct CBlight
{
	glm::vec3 LightColor;
	int IsWorldMesh;
	glm::vec3 LightPos;
	float pad2;
};

struct D3DModel
{
	ID3D11Buffer* VertexBuffer = nullptr;
	UINT Stride = 0;
	UINT Offset = 0;
	UINT NumVertices = 0;
	~D3DModel()
	{
		if (VertexBuffer != nullptr)
			VertexBuffer->Release();
	}
};

struct FrameBufferInfo
{
	ID3D11Texture2D* FrameTexture = nullptr;
	ID3D11RenderTargetView* RenderTargetView = nullptr;
	ID3D11Texture2D* DepthStencilBuffer = nullptr;
	ID3D11DepthStencilView* DepthStencilView = nullptr;
	ID3D11ShaderResourceView* ShaderResourceView = nullptr;
	unsigned int Width = 0;
	unsigned int Height = 0;

	~FrameBufferInfo()
	{
		if (RenderTargetView != nullptr)
			RenderTargetView->Release();
		if (DepthStencilBuffer != nullptr)
			DepthStencilBuffer->Release();
		if (DepthStencilView != nullptr)
			DepthStencilView->Release();
		if (ShaderResourceView != nullptr)
			ShaderResourceView->Release();
		if (FrameTexture != nullptr)
			FrameTexture->Release();
	}
};

HRESULT CreateDepthStencil(FrameBufferInfo& _eye, ID3D11Device*& _device);
HRESULT CreateRendertarget(ID3D11Resource* _resource, ID3D11RenderTargetView** _renderTargetView, ID3D11Device*& _device);
HRESULT CreateFrameBuffer_Internal(FrameBufferInfo& _eye, int _w, int _h, ID3D11Device*& _device);
HRESULT CreateVertexBuffer_Internal(void* _data, D3DModel& _model, UINT _elementSize, UINT _elements, ID3D11Device*& _device);
HRESULT CreateConstantBuffer(void* _memory, UINT _size, ID3D11Buffer** _buffer, ID3D11Device* _device, D3D11_USAGE _usage = D3D11_USAGE_DYNAMIC);
void SetViewport(const FrameBufferInfo& _eye, UINT _width, UINT _height, ID3D11DeviceContext*& _deviceContext);

void ReportError(HRESULT hr, int _line, const char* _function);

#define HR_CHECK(input) \
{ \
	HRESULT rr = input; \
	if (rr < 0) \
	{ \
		ReportError(rr,__LINE__,__FUNCTION__); \
		return rr; \
	} \
} \

//
//void D3DdebugPrint()
//{
//	ID3D11InfoQueue* debug_info_queue;
//	m_device->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&debug_info_queue);
//
//	UINT64 message_count = debug_info_queue->GetNumStoredMessages();
//
//	for (UINT64 i = 0; i < message_count; i++)
//	{
//		SIZE_T message_size = 0;
//		debug_info_queue->GetMessage(i, nullptr, &message_size); //get the size of the message
//
//		D3D11_MESSAGE* message = (D3D11_MESSAGE*)malloc(message_size); //allocate enough space
//		debug_info_queue->GetMessage(i, message, &message_size); //get the actual message
//
//		if (message != NULL)
//		{
//			//do whatever you want to do with it
//			printf("Directx11: %zd %s\n", message->DescriptionByteLength, message->pDescription);
//		}
//		else
//		{
//			printf("Failed to get error msg\n");
//		}
//
//		free(message);
//	}
//
//	debug_info_queue->ClearStoredMessages();
//}
