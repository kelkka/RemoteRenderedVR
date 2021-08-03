#pragma once

#define GLM_FORCE_RADIANS

#include "Engine.h"
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

#include "../NvCodec/Encode/EncoderGL.h"
#include "VRInputs.h"
#include "../Network/Server.h"
#include "../Utility/Timers.h"
#include <ResolutionSettings.h>
#include <VertexDataLens.h>
#include <TextRenderer.h>
#include "../Render/CompressorMap.h"
#include "../Render/RenderHelper.h"

class MainEngineGL: public Engine
{
public:

	MainEngineGL(int argc, char *argv[]);
	~MainEngineGL();
	bool Init();
	void Shutdown();
	void GameLoop();

private:



	//Shared functions used on both client and server

	bool InitGL();

	void DeleteAllShaders();
	void Update(float _dt);
	void Draw(float _dt);
	void SaveThisFrame(GLuint _texture, const char* _id, int _frameBufferID);
	void AlarmTest();
	void LossTest();

	void WaitClientInput();

	bool HandleInput(float _dt);
	void CheckInput(int keyCode, bool& bRet, glm::vec3& debugMove);
	void PrintFrame();
	void SetBitrate(int _rate);
	bool SetupStereoRenderTargets();
	void SetupCompanionWindow();
	void RenderCompanionWindow();
	void CreateAllShaders();

	//Used only on the server

	void WaitForResolution();
	void CopyToCuda(long long _t_begin, long long _t_input);
	void RenderDistortion(vr::Hmd_Eye nEye);
	void SetupDistortion();
	void SetResLevel(int _level);
	void SetupScene();
	void RenderStereoTargets(float _dt);
	void RenderEye(const FramebufferDesc & _eye, const glm::ivec2 & renderSize, float _dt, vr::EVREye _eyeId);
	void RenderScene(vr::Hmd_Eye nEye, float _dt);
	void RenderCube(glm::vec3 &currentPos, const vr::Hmd_Eye &nEye, const GLuint &texture, const glm::vec3& _scale, float _repeat, float _normals = 1.0f);

	//Old tests
	bool ShouldRecord();
	void ForgetTest();
	void BothFactorsTest();

	void CreateStencil();
	void RenderStencil(vr::EVREye _eye);

	EncoderGL* m_encoder[2] = { nullptr,nullptr };
	DecLowLatency* m_decoder[2] = { nullptr,nullptr };
	NetworkEngine* m_netEngine[2] = { nullptr,nullptr };

	static const int FRAME_BUFFERS_ENCODING = 6;
	int m_renderBufferIndex = -1;

	static const int CUBES = 6;
	GLuint m_cubeTextures[CUBES];

	//Encoder stuff
	GLuint m_glEncoderBufferEye[FRAME_BUFFERS_ENCODING][2];
	cudaGraphicsResource* m_cudaGraphicsResourceEye[FRAME_BUFFERS_ENCODING][2] = { nullptr,nullptr };

	unsigned int m_resLevel_Server = 0;
	unsigned int m_frameCounterString = 0;

	CUdevice m_cuDevice = 0;
	CUcontext m_cuEncContext[2] = { 0,0 };

	GLuint m_unLensVAO = 0;
	GLuint m_glIDVertBuffer = 0;
	GLuint m_glIDIndexBuffer = 0;
	unsigned long long m_uiIndexSize;

	TextRenderer& m_textRenderer = TextRenderer::GetInstance();

	std::ofstream m_fileOutput;

	unsigned char* m_frameSavingData = nullptr;

	bool m_bDebugOpenGL=false;
	bool m_quit = false;

	Server* m_server = nullptr;
	VRInputs* m_VRInputs = nullptr;

	SDL_Window * m_pCompanionWindow = nullptr;
	uint32_t m_nCompanionWindowWidth = 0;
	uint32_t m_nCompanionWindowHeight = 0;

	SDL_GLContext m_pContext = 0;

	std::vector<GLuint> m_textures;

	unsigned int m_uiVertcount = 0;

	GLuint m_glSceneVertBuffer;
	GLuint m_unSceneVAO;
	GLuint m_unCompanionWindowVAO;
	GLuint m_glCompanionWindowIDVertBuffer;
	GLuint m_glCompanionWindowIDIndexBuffer;

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

	FramebufferDesc m_leftEye[FRAME_BUFFERS_ENCODING];
	FramebufferDesc m_rightEye[FRAME_BUFFERS_ENCODING];
	FramebufferDesc m_copyBuffer;
	double m_lastMSE = 0;


	uint32_t m_width = 0;
	uint32_t m_height = 0;

	std::vector<ShaderHandler*> m_shaders;

	GLuint m_stencilVAO[2];
	GLuint m_stencilBuffer[2];

	//#define DO_GL_ERROR_CHECKS
	bool MOVING_SCENE = false;
	bool COMPANION_WINDOW = false;
	bool NO_LENS_DISTORTION = false;
	bool FIXED_FRAME_RATE = false;
	bool USE_COMPRESSOR_MAP = false;

	bool m_specialTestButton = false;

	GLuint TEXTURE_RED = 0;
	GLuint TEXTURE_BLUE = 0;
	GLuint TEXTURE_GRAY = 0;
	GLuint TEXTURE_YELLOW = 0;
	GLuint TEXTURE_CHECKER = 0;

	CompressorMap m_compressorMap;

	GLuint m_timeQuery = 0;
	GLuint64 m_cudaMemCpyTime = 0;
};

