#pragma once
//#define LOCAL_TESTING_ONLY

#include <glm.hpp>

#include <SDL.h>
#include "D3DShader.h"
#include <vector>
#include "../Core/VRInputs.h"
#include "D3DCompressor.h"
#include "D3DHelper.h"



class D3DRenderer
{
public:

	~D3DRenderer();
	void DrawCubesBegin(const FrameBufferInfo& _eye, const D3DModel& _model, ID3D11DeviceContext* _deviceContext);
	void Present();

	void SetNoCull(ID3D11DeviceContext* _deviceContext);
	void DrawCube(vr::Hmd_Eye _eye, float _normalDir, const glm::vec3& _pos, const glm::vec3& _scale, ID3D11ShaderResourceView* _textureShaderResourceView, ID3D11DeviceContext* _deviceContext, const D3DModel& _model, float _repeat);
	void EndDraw();
	void DrawCompanionWindow();
	HRESULT InitD3D(int _width, int _height);
	void TimerStart();
	long long TimerEnd();
	ID3D11Device*& GetDevice();
	ID3D11DeviceContext*& GetDeviceContext();
	HRESULT LoadTexture(const char* _path, ID3D11ShaderResourceView** _shaderRscView, D3D11_TEXTURE_ADDRESS_MODE _addressMode);
	HRESULT CreateShaders();
	HRESULT CreateVertexBuffer(void* _data, D3DModel& _model, UINT _elementSize, UINT _elements);

	void SetVRInput(VRInputs* _input) { m_VRInputs = _input; }
	HRESULT CreateFrameBuffer(FrameBufferInfo& _eye, int _w, int _h);
	HRESULT CreateTexture2D(const D3D11_TEXTURE2D_DESC* _desc, ID3D11Texture2D** _tex);

	HRESULT CopyTexture(ID3D11Texture2D* _dest, ID3D11Texture2D* _src, unsigned char* _outData, int _w, int _h, int _channels);
	ID3D11DeviceContext* GetDeferredDeviceContext();
	void RenderDepthStencil(FrameBufferInfo& _frameBuffer, int _eyeIndex);
	const FrameBufferInfo& GetCompanionWindow();
	void D3DRenderer::SetCullFront(bool _cullFront, ID3D11DeviceContext* _deviceContext);
private:


	HRESULT CreateRasterizerState(ID3D11DeviceContext* _deviceContext);
	void DeleteAllShaders();
	HRESULT CreateDevice();
	HRESULT CreateDepthStencilStates();

	ID3D11DeviceContext* m_deviceContext = nullptr;
	ID3D11Device* m_device = nullptr;

	
	ID3D11RasterizerState* m_rasterizerStateBackCull = nullptr;
	ID3D11RasterizerState* m_rasterizerStateFrontCull = nullptr;
	ID3D11RasterizerState* m_rasterizerStateCCW = nullptr;
	ID3D11RasterizerState* m_rasterizerStateNoCull = nullptr;

	ID3D11Buffer* m_cbLightBuffer = nullptr;
	ID3D11Buffer* m_cbModelBuffer = nullptr;
	ID3D11SamplerState* m_samplerState = nullptr;

	ID3D11DepthStencilState* m_depthStencilState = nullptr;
	ID3D11DepthStencilState* m_depthStencilStateInitialize = nullptr;
	ID3D11DeviceContext* m_deferredContext = nullptr;

	int m_width = 0;
	int m_height = 0;

	//CBcamera m_cbCamera;
	CBlight m_cbLight;
	CBmodel m_cbModel;

	SDL_Window* m_pCompanionWindow = nullptr;
	uint32_t m_companionWindowWidth = 0;
	uint32_t m_companionWindowHeight = 0;

	std::vector<D3DShader*> m_shaders;

	FrameBufferInfo m_companionFb;

	VRInputs* m_VRInputs;

#ifdef LOCAL_TESTING_ONLY
	ID3D11Texture2D* m_backBuffer = nullptr;
	IDXGISwapChain1* m_swapChain = nullptr;
#endif

	static const int NUM_QUERIES = 2;
	ID3D11Query* m_disjointQueries[2];
	ID3D11Query* m_timerQueries[NUM_QUERIES][2];
	int m_currentQuery = 0;
};

