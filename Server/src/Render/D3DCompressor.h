#pragma once
#include <glm.hpp>
#include "RenderHelper.h"
#include "D3DHelper.h"
#include "D3DShader.h"

class D3DCompressor
{
public:

	struct Pixel
	{
		unsigned short X;
		unsigned short Y;
	};

	D3DCompressor();
	~D3DCompressor();

	HRESULT LoadMaps(ID3D11Device* _device, ID3D11DeviceContext* _deviceContext);

	HRESULT LoadTexture_Internal(std::string path, int _eye);
	HRESULT LoadShader();

	void Render(const FrameBufferInfo& _frameBuffer, int _eye);

	ID3D11Texture2D* GetEncodedTexture(int _eye);
	const FrameBufferInfo& GetFrameBuffer(int _eye);
	void CreateQuad(float t);
	
	const glm::ivec2& GetCompressedSize();

private:
	HRESULT MakeTexture(int _eye, FILE* _file);

	glm::ivec2 m_encodedSize;
	glm::ivec2 m_decodedSize;
	D3DShader* m_shader = nullptr;

	FrameBufferInfo m_frameBuffer[2];
	ID3D11ShaderResourceView* m_compressionTexture[2] = { nullptr,nullptr };

	ID3D11SamplerState* m_samplerState = nullptr;
	ID3D11Device* m_device = nullptr;
	ID3D11DeviceContext* m_deviceContext = nullptr;
	ID3D11Buffer* m_cbExtra = nullptr;
	ID3D11DepthStencilState* m_depthStencilState = nullptr;


	D3DModel m_quad;

	//int m_eye = 0;

};

