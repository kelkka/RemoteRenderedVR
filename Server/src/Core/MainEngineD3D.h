#pragma once

#define GLM_FORCE_RADIANS
#define D3D_ENCODER

#include <gtc/quaternion.hpp>
#include <stdio.h>
#include <string>
#include <cstdlib>
#include <fstream>
#include <thread>

#include "Engine.h"
#include "VRInputs.h"
#include "../Network/Server.h"
#include "../Utility/Timers.h"
#include <ResolutionSettings.h>
#include <VertexDataLens.h>
#include "../Render/CompressorMap.h"
#include <SDL.h>
#include "../Render/D3DRenderer.h"
//#include "../NvCodec/Encode/D3DEncCUDA.h"
//#include "../NvCodec/Encode/EncLowLatency_v11.h"
#include "../NvCodec/Encode/EncoderD3D.h"

//#include <AssetLoader/include/AssetLoader.h>

class MainEngineD3D : public Engine
{
public:

	MainEngineD3D(int argc, char *argv[]);
	~MainEngineD3D();
	bool Init();
	void Shutdown();
	void GameLoop();

private:



	//Shared functions used on both client and server

	bool InitGL();

	void Update(float _dt);
	void Draw(float _dt);
	void SaveThisFrame(const char* _id);
	void AlarmTest();
	void LossTest();

	void WaitClientInput();

	bool HandleInput(float _dt);
	void CheckInput(int keyCode, bool& bRet, glm::vec3& debugMove);
	void PrintFrame();
	void SetBitrate(int _rate);
	bool SetupStereoRenderTargets();

	//Used only on the server

	void WaitForResolution();
	void CopyToCuda(long long _t_begin, long long _t_render, long long _t_input);
	void SetupDistortion();
	void SetResLevel(int _level);
	void SetupScene();
	void RenderStereoTargets(float _dt);
	void RenderEye(const glm::ivec2& renderSize, float _dt, vr::EVREye _eyeId);
	void RenderScene(vr::Hmd_Eye nEye, float _dt);

	//Old tests
	bool ShouldRecord();
	void CreateStencil();


	static const int FRAME_BUFFERS_ENCODING = 6;
	int m_compressorBufferIndex = -1;
	int m_renderBufferIndex = -1;
	FrameBufferInfo m_eye[FRAME_BUFFERS_ENCODING][2];

	static const int CUBES = 6;
	GLuint m_cubeTextures[CUBES];

	unsigned int m_resLevel_Server = 0;
	unsigned int m_frameCounterString = 0;

	unsigned long long m_uiIndexSize;

	std::ofstream m_fileOutput;

	unsigned char* m_frameSavingData = nullptr;

	bool m_quit = false;

	Server* m_server = nullptr;
	VRInputs* m_VRInputs = nullptr;

	uint32_t m_nCompanionWindowWidth = 0;
	uint32_t m_nCompanionWindowHeight = 0;



	unsigned int m_uiVertcount = 0;

	struct VertexDataScene
	{
		glm::vec3 position;
		glm::vec2 texCoord;
		glm::vec3 normal;
	};

	struct VertexDataWindow
	{
		glm::vec2 position;
		glm::vec2 texCoord;

		VertexDataWindow(const glm::vec2 & pos, const glm::vec2 tex) : position(pos), texCoord(tex) {	}
	};

	uint32_t m_width = 0;
	uint32_t m_height = 0;

	//#define DO_GL_ERROR_CHECKS
	bool MOVING_SCENE = false;
	bool COMPANION_WINDOW = false;
	bool NO_LENS_DISTORTION = false;
	bool FIXED_FRAME_RATE = false;
	bool USE_COMPRESSOR_MAP = false;
	bool USE_D3D11 = false;
	bool USE_WORLD_MESH = false;

	bool m_specialTestButton = false;

	SDL_Window* m_pCompanionWindow = nullptr;


	/// <summary>
	/// 
	/// </summary>
	D3DRenderer m_renderer;
	D3DCompressor m_compressorMap;
	D3DModel m_cubeModel;
	std::map<std::string, ID3D11ShaderResourceView*> m_textures;

	EncoderD3D* m_encoder[2] = { nullptr,nullptr };
	//D3DEncCUDA* m_encoder[2] = { nullptr,nullptr };
	//EncLowLatency_v11* m_encoder[2] = { nullptr,nullptr };
	NetworkEngine* m_netEngine[2] = { nullptr,nullptr };

	ID3D11Texture2D* m_printingTexture = nullptr;

	cudaGraphicsResource* m_cudaResource[2] = { nullptr,nullptr };

	CUdevice m_cuDevice = 0;
	CUcontext m_cuEncContext[2] = { 0,0 };

	D3DModel m_worldModel;
};

