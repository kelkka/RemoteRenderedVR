#pragma once

#define GLM_FORCE_RADIANS

#include <SDL.h>
#include <GL/glew.h>
#include <cuda_gl_interop.h> //includes GL.h
#include <gtc/quaternion.hpp>
#include <cudaGL.h>
#include <stdio.h>
#include <string>
#include <cstdlib>
#include <fstream>
#include <thread>

#include "../NvCodec/Decode/DecLowLatency.h"
#include "VRInputs.h"
#include "../Network/Client.h"
#include "../Utility/Timers.h"
#include <ResolutionSettings.h>
#include <VertexDataLens.h>
#include <GLShaderHandler.h>
#include "../Render/CompressorMap.h"

class MainEngine
{
public:

	struct FramebufferDesc
	{
		GLuint m_nDepthBufferId;
		GLuint m_nRenderTextureId;
		GLuint m_nRenderFramebufferId;
		GLuint m_nResolveTextureId;
		GLuint m_nResolveFramebufferId;
		glm::ivec2 m_size;
	};

	MainEngine();
	~MainEngine();

	//The server and client used to be in the same file, so there is some weird stuff going on
	//Functions that may happen on both server and client, though likely altered more or less, are called X_
	bool Init();
	void CreateQuadVBO();
	bool InitGL();

	void SetAllResolutions();

	void Shutdown();
	void DeleteAllShaders();
	vr::HmdVector3_t GetPosition(vr::HmdMatrix34_t matrix);
	void Update(float _dt);
	void Draw(float _dt);
	void SaveThisFrame(GLuint _texture, const char* _id);
	void AlarmTest();
	void LossTest();
	vr::HmdQuaternion_t GetRotation(vr::HmdMatrix34_t matrix);
	void GameLoop();
	bool HandleInput(float _dt);
	void CheckInput(int keyCode, bool& bRet, glm::vec3& debugMove);
	void PrintFrame();
	void SetBitrate(int _rate);
	bool SetupStereoRenderTargets();
	void SetupCompanionWindow();
	void GLErrorCheck(const char* _method, int _line);
	void RenderCompanionWindow();
	void CreateAllShaders();
	bool CreateFrameBuffer(int nWidth, int nHeight, FramebufferDesc& framebufferDesc, bool resolveOnly);

	//Used only on client, called Cl_
	void SendHeadsetInfo();
	bool InitCompositor();
	void TransmitInputMatrix(float _dt, bool _force);
	void SetupDistortion();
	void SendHiddenAreaMesh();
	bool RenderCudaToGL();

	bool CopyImageFromDecoder(int _eye);

	void ComputeDecompression(GLuint _decoderTexture, GLuint _resolveTexture, GLuint _compressionTexture);
	void ExperimentLogic();
	void DecodedTextureResize();

	void PostDecodeDelayDecision();
	void ProcessVREvent(const vr::VREvent_t& event);
	void ComputeMSE(bool _freeze);
	void WriteHiddenAreaMesh(const char* _path, const vr::HiddenAreaMesh_t& _mesh);

	void ReadHiddenAreaMesh(const char* _path, vr::HiddenAreaMesh_t& _mesh);
private:

	//Config
	bool MOVING_SCENE = false;
	bool COMPANION_WINDOW = false;
	bool SOAK_TEST = false;
	bool RESOLUTION_TEST = false;
	bool BITRATE_TEST = false;
	bool LOSS_TEST = false;
	bool NO_VR = true;
	bool NO_LENS_DISTORTION = false;
	bool TWITCH_TEST = true;
	int NUM_FRAMES_TO_TEST = 500;
	int MAX_BITRATE_TO_TEST = 0;
	double RESOLUTION_FACTOR = 0.20;
	bool FIXED_FRAME_RATE = false;
	bool ROTATION_FROM_FILE_TEST = false;
	bool PRINT_EVERY_FRAME = false;
	bool USE_SUPER_RESOLUTION = false;
	bool USE_COMPRESSOR_MAP = false;
	bool USE_D3D11 = false;
	bool INCREASING_ROTATION_TEST = false;

	//MSE
	int m_MSEBufferSize = 32 * 32 * 16 * 16;
	void* m_MSEBufferPtr = 0;
	GLuint m_MSEBuffer = 0;
	double m_lastMSE = 0;

	//Decoder stuff
	GLuint m_glDecoderPBO[2];
	GLuint m_glDecoderTexture[2];
	CUgraphicsResource m_cuDecResource[2];
	DecLowLatency* m_decoder[2] = { nullptr,nullptr };
	CUdevice m_cuDevice = 0;
	CUcontext m_cuDecContext[2] = { 0,0 };

	//Network
	Client m_client;

	//Input
	VRInputs* m_VRInputs;

	// SDL bookkeeping
	SDL_Window* m_pCompanionWindow;
	uint32_t m_nCompanionWindowWidth;
	uint32_t m_nCompanionWindowHeight;
	SDL_GLContext m_pContext;

	// OpenGL bookkeeping

	//Quad
	GLuint m_2DVAO;
	GLuint m_2DVBO[2];

	//Shaders
	GLuint m_shaderCube;
	GLuint m_shaderCompanion;
	GLuint m_shaderMSE;
	GLuint m_shaderQuadDec;
	GLuint m_shaderDistort;
	GLuint m_shaderDecompress;
	std::vector<ShaderHandler*> m_shaders;

	//Companion window
	GLuint m_unCompanionWindowVAO;
	GLuint m_glCompanionWindowIDVertBuffer;
	GLuint m_glCompanionWindowIDIndexBuffer;
	unsigned int m_uiCompanionWindowIndexSize;

	//Buffers
	FramebufferDesc m_leftEye;
	FramebufferDesc m_rightEye;
	FramebufferDesc m_copyBuffer;

	const int FRAME_BUFFERS = 5;
	int COMPRESSOR_BUFFER_INDEX = 5;

	//Random stuff

	uint32_t m_width = 0;
	uint32_t m_height = 0;

	//Super resolution
	//uint32_t m_widthSR = 0;
	//uint32_t m_heightSR = 0;

	unsigned char* m_frameSavingData[2] = { nullptr };

	bool m_bDebugOpenGL = 0;
	bool m_quit = false;

	vr::IVRSystem* m_pHMD;
	bool m_specialTestButton = false;
	int m_clFrameIndex[2] = { -1,-1 };

	bool m_kickOffDelay = 0;
	int m_predictFramesAhead = 0;

	//SuperScale* m_superScale = nullptr;
	//unsigned char* m_superScaleImageMem = nullptr;

	CompressorMap m_compressorMap;

	GLuint m_timeQuery = 0;
	GLuint64 m_displayTime = 0;
};
