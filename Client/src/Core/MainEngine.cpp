#include "MainEngine.h"
#include <CudaError.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image_write.h>
#include <stb_image.h>
#include "..\NvCodec\Decode\DecOpenH264.h"

//#define DO_GL_ERROR_CHECKS


// Constructor

MainEngine::MainEngine()
{

}

// Destructor

MainEngine::~MainEngine()
{
	printf("Shutdown");
}

std::string Cl_GetTrackedDeviceString(vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError* peError = NULL)
{
	uint32_t unRequiredBufferLen = vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, prop, NULL, 0, peError);
	if (unRequiredBufferLen == 0)
		return "";

	char* pchBuffer = new char[unRequiredBufferLen];
	unRequiredBufferLen = vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
	std::string sResult = pchBuffer;
	delete[] pchBuffer;
	return sResult;
}


// 

bool MainEngine::Init()
{
	RESOLUTION_TEST = StatCalc::GetParameter("RESOLUTION_TEST") > 0;
	SOAK_TEST = StatCalc::GetParameter("SOAK_TEST") > 0;
	LOSS_TEST = StatCalc::GetParameter("LOSS_TEST") > 0;
	NUM_FRAMES_TO_TEST = (int)StatCalc::GetParameter("NUM_FRAMES_TO_TEST");
	RESOLUTION_FACTOR = StatCalc::GetParameter("RESOLUTION_FACTOR");
	NO_VR = StatCalc::GetParameter("NO_VR") > 0;
	COMPANION_WINDOW = StatCalc::GetParameter("COMPANION_WINDOW") > 0;
	BITRATE_TEST = StatCalc::GetParameter("BITRATE_TEST") > 0;
	MAX_BITRATE_TO_TEST = (int)StatCalc::GetParameter("MAX_BITRATE_TO_TEST");
	NO_LENS_DISTORTION = StatCalc::GetParameter("NO_LENS_DISTORTION") > 0;
	TWITCH_TEST = StatCalc::GetParameter("TWITCH_TEST") > 0;
	ROTATION_FROM_FILE_TEST = StatCalc::GetParameter("ROTATION_FROM_FILE_TEST") > 0;
	PRINT_EVERY_FRAME = StatCalc::GetParameter("PRINT_EVERY_FRAME") > 0;
	USE_SUPER_RESOLUTION = StatCalc::GetParameter("USE_SUPER_RESOLUTION") > 0;
	USE_COMPRESSOR_MAP = StatCalc::GetParameter("USE_COMPRESSOR_MAP") > 0;
	USE_D3D11 = StatCalc::GetParameter("USE_D3D11") > 0;
	INCREASING_ROTATION_TEST = StatCalc::GetParameter("INCREASING_ROTATION_TEST") > 0;

	m_width = (int)StatCalc::GetParameter("EYE_WIDTH");
	m_height = (int)StatCalc::GetParameter("EYE_HEIGHT");

	m_nCompanionWindowHeight = 1080;
	m_nCompanionWindowWidth = uint32_t(m_nCompanionWindowHeight * 0.9f) * 2;


	//if (USE_SUPER_RESOLUTION)
	//{
	//	m_superScale = new SuperScale();
	//	m_superScale->Init("X:/Desk/Git/OpenCV/models/ESPCN_x2.pb", "espcn");
	//}

	if (m_client.InitNetwork() < 0)
		return false;

	int connected = -1;

	while (connected < 0)
	{
		connected = m_client.Connect();

		if (connected == 0)
			break;

		//std::string temp;
		//std::cin >> temp;

		//if (temp == "q")
		//	break;

		printf("Retrying\n");
	}

	// Loading the SteamVR Runtime
	if (!NO_VR)
	{
		vr::EVRInitError eError = vr::VRInitError_None;

		m_pHMD = vr::VR_Init(&eError, vr::VRApplication_Scene);

		if (eError != vr::VRInitError_None)
		{
			m_pHMD = NULL;
			char buf[1024];
			sprintf_s(buf, sizeof(buf), "Unable to init VR runtime: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "VR_Init Failed", buf, NULL);
			return false;
		}

		m_VRInputs = new VRInputs(m_pHMD);

	}
	else
	{
		m_VRInputs = new VRInputs(NULL);
	}

	int nWindowPosX = 700;
	int nWindowPosY = 100;
	Uint32 unWindowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);

	if (m_bDebugOpenGL)
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

	SetProcessDPIAware();

	const char* name = "Client";

	m_pCompanionWindow = SDL_CreateWindow(name, nWindowPosX, nWindowPosY, m_nCompanionWindowWidth, m_nCompanionWindowHeight, unWindowFlags);

	if (COMPANION_WINDOW)
	{
		SDL_WarpMouseInWindow(m_pCompanionWindow, m_nCompanionWindowWidth / 2, m_nCompanionWindowHeight / 2);
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);

	if (m_pCompanionWindow == NULL)
	{
		printf("%s - Window could not be created! SDL Error: %s\n", __FUNCTION__, SDL_GetError());
		return false;
	}

	m_pContext = SDL_GL_CreateContext(m_pCompanionWindow);
	if (m_pContext == NULL)
	{
		printf("%s - OpenGL context could not be created! SDL Error: %s\n", __FUNCTION__, SDL_GetError());
		return false;
	}

	glewExperimental = GL_TRUE;
	GLenum nGlewError = glewInit();
	if (nGlewError != GLEW_OK)
	{
		printf("%s - Error initializing GLEW! %s\n", __FUNCTION__, glewGetErrorString(nGlewError));
		return false;
	}
	glGetError(); // to clear the error caused deep in GLEW

	int vsync = 1;

	if (SDL_GL_SetSwapInterval(vsync) < 0)
	{
		printf("%s - Warning: Unable to set VSync! SDL Error: %s\n", __FUNCTION__, SDL_GetError());
		//return false;
	}

	std::string m_strDriver = "No Driver";
	std::string m_strDisplay = "No Display";

	if (!NO_VR)
	{
		m_strDriver = Cl_GetTrackedDeviceString(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String);
		m_strDisplay = Cl_GetTrackedDeviceString(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String);

		std::string strWindowTitle = name + (std::string)" " + m_strDriver + " " + m_strDisplay;
		SDL_SetWindowTitle(m_pCompanionWindow, strWindowTitle.c_str());
	}

	if (!InitGL())
	{
		printf("%s - Unable to initialize OpenGL!\n", __FUNCTION__);
		return false;
	}

	if (!NO_VR)
	{
		if (!InitCompositor())
		{
			printf("%s - Failed to initialize VR Compositor!\n", __FUNCTION__);
			return false;
		}
	}

	GLErrorCheck(__func__, __LINE__);

	Timers::Get().Init(m_width, m_height);

	m_client.SetVRInput(m_VRInputs);

	//Send HMD projection matrices etc to server.
	SendHeadsetInfo();

	return true;
}



//			Outputs the string in message to debugging output.
//          All other parameters are ignored.
//          Does not return any meaningful value or reference.

void APIENTRY DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam)
{
	printf("GL Error: %s\n", message);
}



// Initialize OpenGL. Returns true if OpenGL has been successfully
//          initialized, false if shaders could not be created.
//          If failure occurred in a module other than shaders, the function
//          may return true or throw an error. 

bool MainEngine::InitGL()
{
	if (m_bDebugOpenGL)
	{
		glDebugMessageCallback((GLDEBUGPROC)DebugCallback, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	}

	if (!NO_VR)
	{
		uint32_t w, h;

		m_pHMD->GetRecommendedRenderTargetSize(&w, &h);
		printf("Recommended size: %dx%d\n", w, h);
	}

	if (USE_COMPRESSOR_MAP)
	{
		m_compressorMap.LoadMaps();
	}

	SetAllResolutions();

	CreateAllShaders();

	m_VRInputs->SetupCameras(m_width, m_height);

	if (!SetupStereoRenderTargets())
		return false;

	SetupDistortion();

	SetupCompanionWindow();

	GLErrorCheck(__func__, __LINE__);

	glGenBuffers(1, &m_MSEBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_MSEBuffer);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, m_MSEBufferSize * sizeof(double), 0, GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

	m_MSEBufferPtr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, m_MSEBufferSize * sizeof(double), GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

	CreateQuadVBO();


	
	glGenQueries(1, &m_timeQuery);

	glClearColor(0,0,0,1);

	GLErrorCheck(__func__, __LINE__);

	return true;
}


void MainEngine::SetAllResolutions()
{
	float imageSize = 1;

	for (int j = 0; j < FRAME_BUFFERS; j++)
	{
		imageSize = (float)(RESOLUTION_FACTOR * (5 - j));

		//imageSize = 1.0f - j * 0.01f;
		int width = int(m_width * imageSize);
		int height = int(m_height * imageSize);

		int rw = width % 16;
		int rh = height % 16;

		if (rw > 0)
			width = width + 16 - rw;

		if (rh > 0)
			height = height + 16 - rh;

		if (j == 0)
		{
			m_width = width;
			m_height = height;
			printf("Used size: %dx%d\n", m_width, m_height);
		}

		int level = ResolutionSettings::Get().AddResolution(glm::ivec2(width, height));
		printf("Creating Images with size: (%d) %dx%d %.2f%%\n", level, width, height, imageSize * 100);
	}

	if (USE_COMPRESSOR_MAP)
	{
		const glm::ivec2& compressorSize = m_compressorMap.GetCompressedSize();
		//glm::ivec2 compressorSize = glm::ivec2(1520, 1360);
		COMPRESSOR_BUFFER_INDEX = ResolutionSettings::Get().AddResolution(compressorSize);
		printf("Adding compression buffer (%d) %d,%d\n", COMPRESSOR_BUFFER_INDEX, compressorSize.x, compressorSize.y);
	}

	if (m_frameSavingData[0] == nullptr)
	{
		m_frameSavingData[0] = (unsigned char*)malloc(2048 * 2048 * 4ULL);
		m_frameSavingData[1] = (unsigned char*)malloc(2048 * 2048 * 4ULL);
	}

	//if (USE_SUPER_RESOLUTION)
	//{
	//	m_superResolution = (unsigned char*)malloc(m_width * m_height * 4);
	//}
}


// 

void MainEngine::Shutdown()
{
	free(m_frameSavingData[0]);
	free(m_frameSavingData[1]);

	m_client.Destroy();

	if (m_pHMD)
	{
		vr::VR_Shutdown();
		m_pHMD = NULL;
	}

	if (m_pContext)
	{
		if (m_bDebugOpenGL)
		{
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_FALSE);
			glDebugMessageCallback(nullptr, nullptr);
		}

		DeleteAllShaders();

		glDeleteRenderbuffers(1, &m_leftEye.m_nDepthBufferId);
		glDeleteTextures(1, &m_leftEye.m_nRenderTextureId);
		glDeleteFramebuffers(1, &m_leftEye.m_nRenderFramebufferId);
		glDeleteTextures(1, &m_leftEye.m_nResolveTextureId);
		glDeleteFramebuffers(1, &m_leftEye.m_nResolveFramebufferId);

		//cudaGraphicsUnregisterResource(leftEyeDesc[_eye].m_cudaGraphicsResource);

		glDeleteRenderbuffers(1, &m_rightEye.m_nDepthBufferId);
		glDeleteTextures(1, &m_rightEye.m_nRenderTextureId);
		glDeleteFramebuffers(1, &m_rightEye.m_nRenderFramebufferId);
		glDeleteTextures(1, &m_rightEye.m_nResolveTextureId);
		glDeleteFramebuffers(1, &m_rightEye.m_nResolveFramebufferId);

		//cudaGraphicsUnregisterResource(m_rightEye[_eye].m_cudaGraphicsResource);

		for (int i = 0; i < 2; i++)
		{
			CudaErr(cuGraphicsUnregisterResource(m_cuDecResource[i]));
		}

		glDeleteBuffers(1, &m_MSEBuffer);

		for (int i = 0; i < 2; i++)
		{
			if (m_decoder[i] != nullptr)
			{
				if(StatCalc::GetParameter("NVDEC") > 0)
					delete (DecLowLatency*)m_decoder[i];
				else
					delete (DecOpenH264*)m_decoder[i];
			}
		}

		//glDeleteTextures(1, &m_copyBuffer.m_nResolveTextureId);
		//glDeleteFramebuffers(1, &m_copyBuffer.m_nResolveFramebufferId);

		glDeleteBuffersARB(2, m_glDecoderPBO);
		glDeleteTextures(2, m_glDecoderTexture);

		if (m_unCompanionWindowVAO != 0)
		{
			glDeleteVertexArrays(1, &m_unCompanionWindowVAO);
		}

		glDeleteBuffers(2, m_2DVBO);
	}

	if (m_pCompanionWindow)
	{
		SDL_DestroyWindow(m_pCompanionWindow);
		m_pCompanionWindow = NULL;
	}

	//if (m_superScale != nullptr)
	//{
	//	delete m_superScale;
	//}
	//if (m_superScaleImageMem != nullptr)
	//{
	//	delete m_superScaleImageMem;
	//}

	glDeleteQueries(1, &m_timeQuery);

	delete m_VRInputs;

	SDL_Quit();
}

void MainEngine::DeleteAllShaders()
{
	for (size_t i = 0; i < m_shaders.size(); i++)
	{
		delete m_shaders[i];
	}

	m_shaders.clear();
}

vr::HmdVector3_t MainEngine::GetPosition(vr::HmdMatrix34_t matrix)
{
	vr::HmdVector3_t vector;

	vector.v[0] = matrix.m[0][3];
	vector.v[1] = matrix.m[1][3];
	vector.v[2] = matrix.m[2][3];

	return vector;
}

vr::HmdQuaternion_t MainEngine::GetRotation(vr::HmdMatrix34_t matrix)
{
	vr::HmdQuaternion_t q;

	q.w = sqrt(fmax(0, 1 + matrix.m[0][0] + matrix.m[1][1] + matrix.m[2][2])) / 2;
	q.x = sqrt(fmax(0, 1 + matrix.m[0][0] - matrix.m[1][1] - matrix.m[2][2])) / 2;
	q.y = sqrt(fmax(0, 1 - matrix.m[0][0] + matrix.m[1][1] - matrix.m[2][2])) / 2;
	q.z = sqrt(fmax(0, 1 - matrix.m[0][0] - matrix.m[1][1] + matrix.m[2][2])) / 2;
	q.x = copysign(q.x, matrix.m[2][1] - matrix.m[1][2]);
	q.y = copysign(q.y, matrix.m[0][2] - matrix.m[2][0]);
	q.z = copysign(q.z, matrix.m[1][0] - matrix.m[0][1]);
	return q;
}


// 

bool MainEngine::HandleInput(float _dt)
{
	bool bRet = false;
	//#ifdef NO_VR
	//	static glm::vec3 debugMove = glm::vec3(0, -0.760001183f, -6.51999474f); //for 3D scene
	//#else
	//	static glm::vec3 debugMove = glm::vec3(0, 1.0f, -3.0f); //for text
	//#endif
	//
	static glm::vec3 debugMove = glm::vec3(0, 0.5f, 0.0f); //for text

	SDL_Event sdlEvent;

	m_client.SetKeyPress(0);

	while (SDL_PollEvent(&sdlEvent) != 0)
	{
		if (sdlEvent.type == SDL_QUIT)
		{
			bRet = true;
		}
		else if (sdlEvent.type == SDL_KEYDOWN)
		{
			CheckInput(sdlEvent.key.keysym.sym, bRet, debugMove);

			if (NO_VR)
			{
				m_VRInputs->ButtonPress(sdlEvent.key.keysym.sym);
			}
			//Send keypress to server :d
			m_client.SetKeyPress(sdlEvent.key.keysym.sym);
		}
	}

	if (!NO_VR)
	{
		// Process SteamVR events
		vr::VREvent_t event;
		while (m_pHMD->PollNextEvent(&event, sizeof(event)))
		{
			ProcessVREvent(event);
		}
	}

	return bRet;
}

void MainEngine::CheckInput(int keyCode, bool& bRet, glm::vec3& debugMove)
{
	if (keyCode == SDLK_ESCAPE
		|| keyCode == SDLK_q)
	{
		bRet = true;
	}
	if (keyCode == SDLK_c)
	{

	}
	if (keyCode == SDLK_r)
	{
		DeleteAllShaders();
		CreateAllShaders();
	}
	if (keyCode == SDLK_t)
	{
		//TODO
		//m_client.ReadConfig("../content/netConfig_Client.txt");
	}
	if (keyCode == SDLK_s)
	{
		debugMove.y -= 0.02f;

	}
	if (keyCode == SDLK_w)
	{
		debugMove.y += 0.02f;

	}
	if (keyCode == SDLK_d)
	{
		debugMove.z += 0.02f;

	}
	if (keyCode == SDLK_a)
	{
		debugMove.z -= 0.02f;

	}
	if (keyCode == SDLK_z)
	{
		debugMove.x += 0.02f;

	}
	if (keyCode == SDLK_c)
	{
		debugMove.x -= 0.02f;
	}
	if (keyCode == SDLK_y)
	{
		m_specialTestButton = !m_specialTestButton;
		printf("Special button %d\n", m_specialTestButton);
		printf("This starts Loss test\n");
	}
	if (keyCode == SDLK_k)
	{
		m_kickOffDelay = true;
		printf("Add one frame delay\n");
	}
	if (keyCode == SDLK_p)
	{
		PrintFrame();
	}
	if (keyCode == SDLK_m)
	{
		m_predictFramesAhead++;
		printf("Predicting %d frames ahead\n", m_predictFramesAhead);
	}
	if (keyCode == SDLK_n)
	{
		if (m_predictFramesAhead > 0)
		{
			m_predictFramesAhead--;
		}

		printf("Predicting %d frames ahead\n", m_predictFramesAhead);
	}

	if (keyCode == SDLK_1)
	{
		m_client.SetKeyPress(SDLK_1);
		printf("Request resolution level 0\n");
	}
	if (keyCode == SDLK_2)
	{
		m_client.SetKeyPress(SDLK_2);
		printf("Request resolution level 1\n");
	}
	if (keyCode == SDLK_3)
	{
		m_client.SetKeyPress(SDLK_3);
		printf("Request resolution level 2\n");
	}
	if (keyCode == SDLK_4)
	{
		m_client.SetKeyPress(SDLK_4);
		printf("Request resolution level 3\n");
	}
	if (keyCode == SDLK_5)
	{
		m_client.SetKeyPress(SDLK_5);
		printf("Request resolution level 4\n");
	}
}

void MainEngine::PrintFrame()
{
	SaveThisFrame(m_leftEye.m_nResolveTextureId, "L");
	//SaveThisFrame(m_rightEye[0].m_nResolveTextureId, "R");
}

void MainEngine::SetBitrate(int _rate)
{
	if (_rate <= 0)
		return;

	//Bitrate test
	printf("Bitrate is now %d\n", _rate);
}

void MainEngine::Update(float _dt)
{
	m_client.Update();

	if (m_client.IsConnected() == false)
		m_quit = true;

	if (m_specialTestButton)
		LossTest();
}

void MainEngine::AlarmTest()
{
	if (Timers::Get().HasStartedPrinting())
	{
		static double alarm = 0.00;
		static int counter = 1000;
		counter++;

		if (counter > 1000)
		{
			counter = 0;

			if (alarm < 0.85)
				alarm += 0.1;
			else if (alarm < 0.99)
				alarm += 0.01;
			else
				alarm += 0.001;

			StatCalc::SetParameter("GAMMA_P_VAL", alarm);
		}

		if (alarm > 1.0)
		{
			m_quit = true;
		}
	}
}

void MainEngine::LossTest()
{

	static int counter = 0;
	static int loss = 10000;

	counter++;

	if (counter > 1000)
	{
		loss = int(loss * 0.75f);
		counter = 0;
	}

	m_client.SetSimulationLoss(loss);

	if (loss < 10)
	{
		m_quit = true;
	}
}


// Is the game loop

void MainEngine::GameLoop()
{
	//Looped until exiting program with ESC or similar
	while (!m_quit)
	{
		//deltatime calc
		static long long prev = 0;
		long long t0 = Timers::Get().Time();
		long long dt = t0 - prev;
		prev = t0;
		float fdt = dt / 1000000.0f;

		bool inputQuit = HandleInput(fdt);

		if (inputQuit)
			m_quit = true;

		//Frame timing is important for experiments so this should not be moved
		ExperimentLogic();

		//Sends input matrix of this frame to server
		TransmitInputMatrix(fdt, false);

		//Basically only checks for disconnects. But generic logic can be put here.
		Update(fdt);

		//Rendering
		Draw(fdt);
	}
}


// Server draws to both eyes here and sends to encoder. 
// The client renders here from cuda to opengl to quads, one per eye, and submits them to the driver

void MainEngine::Draw(float _dt)
{
	if (RenderCudaToGL() == false)
	{
		//Not ready
		//std::this_thread::sleep_for(std::chrono::microseconds(16000));
		return;
	}

	if (COMPANION_WINDOW)
	{
		RenderCompanionWindow();
	}

	//If experiments need MSE, uncomment this
	//ComputeMSE(freeze);

	Timers& timer = Timers::Get();

	if (!NO_VR)
	{
		int lag = Timers::Get().PrintLag();
		vr::Compositor_FrameTiming timing;
		timing.m_nSize = sizeof(vr::Compositor_FrameTiming);
		bool ok = vr::VRCompositor()->GetFrameTiming(&timing, lag);
		if (ok)
			timer.PushFloat(0, T_MISC, m_clFrameIndex[0] - lag, (int)timing.m_nNumFramePresents - 1);
		else
			printf("ERROR GETTING FRAME TIMING!!\n");

		//Send textures to Compositor (Render in headset)

		//vr::Texture_t leftTex = { (void*)(uintptr_t)m_leftEye[0].m_nResolveTextureId, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		//vr::Texture_t rightTex = { (void*)(uintptr_t)m_rightEye[0].m_nResolveTextureId, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		//if (m_client.GetCurrentLevel() == 0)
		//{
		//	vr::VRCompositor()->Submit(vr::Eye_Left, &leftTex, (const vr::VRTextureBounds_t *)0, vr::Submit_LensDistortionAlreadyApplied);
		//	vr::VRCompositor()->Submit(vr::Eye_Right, &rightTex, (const vr::VRTextureBounds_t *)0, vr::Submit_LensDistortionAlreadyApplied);
		//}
		//else
		//{
		//	vr::VRCompositor()->Submit(vr::Eye_Left, &leftTex, (const vr::VRTextureBounds_t *)0, vr::Submit_Default);
		//	vr::VRCompositor()->Submit(vr::Eye_Right, &rightTex, (const vr::VRTextureBounds_t *)0, vr::Submit_Default);
		//}

		int flags = vr::Submit_Default;

		if (m_predictFramesAhead > 0)
		{
			flags = flags | vr::EVRSubmitFlags::Submit_TextureWithPose;
		}

		if (m_client.GetCurrentLevel() == 0 && !NO_LENS_DISTORTION)
		{
			flags = flags | vr::EVRSubmitFlags::Submit_LensDistortionAlreadyApplied;
		}

		vr::VRTextureWithPose_t leftTex;
		leftTex.eColorSpace = vr::ColorSpace_Gamma;
		leftTex.eType = vr::TextureType_OpenGL;
		leftTex.handle = (void*)(uintptr_t)m_leftEye.m_nResolveTextureId;
		leftTex.mDeviceToAbsoluteTracking = m_VRInputs->GetHMDPoseUsedInPrediction();

		vr::VRTextureWithPose_t rightTex;
		rightTex.eColorSpace = vr::ColorSpace_Gamma;
		rightTex.eType = vr::TextureType_OpenGL;
		rightTex.handle = (void*)(uintptr_t)m_rightEye.m_nResolveTextureId;
		rightTex.mDeviceToAbsoluteTracking = m_VRInputs->GetHMDPoseUsedInPrediction();

		//Sends images to HMD
		vr::VRCompositor()->Submit(vr::Eye_Left, &leftTex, (const vr::VRTextureBounds_t*)0, (vr::EVRSubmitFlags)flags);
		vr::VRCompositor()->Submit(vr::Eye_Right, &rightTex, (const vr::VRTextureBounds_t*)0, (vr::EVRSubmitFlags)flags);
	}

	//Frame finished, record time consumed and so on

	//for (int i = 0; i < 2; i++)
	//{
	//	m_clFrameIndex[i] = m_decoder[i]->GetLastNonRenderedFrameID();
	//}

	if (m_clFrameIndex[0] > -1 && m_clFrameIndex[1] > -1)
	{

		//*Be sure to set timing.size = sizeof(Compositor_FrameTiming) on struct passed in before calling this function. * /


		timer.PushFloat(0, T_FPSx90, m_clFrameIndex[0], _dt * 90.0);
		timer.PushFloat(1, T_FPSx90, m_clFrameIndex[1], _dt * 90.0);
		timer.PushFloat(0, T_RES_FACTOR, m_clFrameIndex[0], RESOLUTION_FACTOR * (5 - m_client.GetCurrentLevel()));
		timer.PushFloat(1, T_RES_FACTOR, m_clFrameIndex[1], RESOLUTION_FACTOR * (5 - m_client.GetCurrentLevel()));

		timer.PushLongLong(0, T_JITTER, m_clFrameIndex[0], m_displayTime / 1000);

// 		timer.PushLongLong(0, T_JITTER, m_clFrameIndex[0], Timers::Get().Time());	//sync wait
// 		timer.PushLongLong(1, T_JITTER, m_clFrameIndex[1], Timers::Get().Time());	//sync wait

		//long long finish = Timers::Get().Time();
		//timer.PushLongLong(1, T_DIS_FIN, m_clFrameIndex[1], finish);
		//timer.PushLongLong(0, T_DIS_FIN, m_clFrameIndex[0], finish);

		timer.Print(m_clFrameIndex[0]);

		printf("Frame: %u	FPS: %f\r", m_clFrameIndex[0], 1.0f / _dt);

		//Resize the companion window too, only needed if wanting screenshots of the true size
//#ifdef COMPANION_WINDOW
//		int currentLevel = m_client.GetCurrentLevel();
//		static int prevRes = currentLevel;
//
//		const glm::vec2 windowSize = ResolutionSettings::Get().Resolution(currentLevel);
//
//		if (prevRes != currentLevel)
//		{
//			SDL_SetWindowSize(m_pCompanionWindow, windowSize.x * 2, windowSize.y);
//			printf("Resized companion window to %dx%d\n", windowSize.x * 2, windowSize.y);
//		}
//		prevRes = currentLevel;
//#endif
	}

	if (COMPANION_WINDOW)
	{
		SDL_GL_SwapWindow(m_pCompanionWindow);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
	else
	{

	}

	GLErrorCheck(__func__, __LINE__);
}

void MainEngine::SaveThisFrame(GLuint _texture, const char* _id)
{
	//memset(m_frameSavingData, 0, m_width * m_height * 4);

	//static unsigned char imgData[1200 * 1080 * 4];
	//memset(imgData, 0, 1200 * 1080 * 4);

	//glBindTexture(GL_TEXTURE_RECTANGLE, m_glDecoderTexture[0]);
	//glGetTexImage(GL_TEXTURE_RECTANGLE, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_frameSavingData);
	//glBindTexture(GL_TEXTURE_RECTANGLE, 0);

	glBindTexture(GL_TEXTURE_2D, m_leftEye.m_nResolveTextureId);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_frameSavingData[0]);
	glBindTexture(GL_TEXTURE_2D, 0);

	GLErrorCheck(__func__, __LINE__);

	std::string name = StatCalc::GetParameterStr("SCREENSHOT_PATH");

	name += "/VR_Cl_";
	//glm::ivec2 size = ResolutionSettings::Get().Resolution(m_client.GetCurrentLevel());
	glm::ivec2 sizePNG = ResolutionSettings::Get().Resolution(0);

	//if (USE_COMPRESSOR_MAP)
	//{
	//	sizePNG = ResolutionSettings::Get().Resolution(FRAME_BUFFERS);
	//}

	name += std::to_string((int)StatCalc::GetParameter("FILE_IDENTIFIER_INT"));
	name += "_";

	name += _id;
	name += "_";

	//RESOLUITION

	//name += std::to_string(size.x);
	//name += "x";
	//name += std::to_string(size.y);

	//name += "_";

	//BITRATE

//	int bitrate = m_client.GetBitRate();

//	name += std::to_string(bitrate);
//	name += "_";

	name += std::to_string(m_clFrameIndex[0]);

	name += ".png";

	if (USE_D3D11)
	{
		stbi_flip_vertically_on_write(1);
	}

	int result = stbi_write_png(name.c_str(), sizePNG.x, sizePNG.y, 4, m_frameSavingData[0], sizePNG.x * 4);
	printf("image saved %s, error %d\n", name.c_str(), result);

	//SuperScale::ScaleMe(name.c_str());
}

void MainEngine::CreateAllShaders()
{
	ShaderHandler* shader = new ShaderHandler();

	shader->CreateShaderProgram();
	shader->AddShader("../content/Shaders/vsCompanion.vert", GL_VERTEX_SHADER);
	shader->AddShader("../content/Shaders/fsCompanion.frag", GL_FRAGMENT_SHADER);
	shader->LinkShaderProgram();

	m_shaderCompanion = (GLuint)m_shaders.size();
	m_shaders.push_back(shader);

	shader = new ShaderHandler();

	shader->CreateShaderProgram();
	shader->AddShader("../content/Shaders/coMSE.comp", GL_COMPUTE_SHADER);
	shader->LinkShaderProgram();

	shader->AddUniform("width");
	shader->AddUniform("height");

	m_shaderMSE = (GLuint)m_shaders.size();
	m_shaders.push_back(shader); 

	shader = new ShaderHandler();

	shader->CreateShaderProgram();
	shader->AddShader("../content/Shaders/vsQuadDec.vert", GL_VERTEX_SHADER);

	if (USE_D3D11)
	{
		shader->AddShader("../content/Shaders/fsQuadDecFlipped.frag", GL_FRAGMENT_SHADER);
	}
	else
	{
		shader->AddShader("../content/Shaders/fsQuadDec.frag", GL_FRAGMENT_SHADER);
	}

	shader->LinkShaderProgram();

	//shader->AddUniform("width");
	//shader->AddUniform("height");

	m_shaderQuadDec = (GLuint)m_shaders.size();
	m_shaders.push_back(shader); 

	shader = new ShaderHandler();

	shader->CreateShaderProgram();
	shader->AddShader("../content/Shaders/coDecompress.comp", GL_COMPUTE_SHADER);
	shader->LinkShaderProgram();

	shader->AddUniform("gEncWidth");
	shader->AddUniform("gEncHeight");

	m_shaderDecompress = (GLuint)m_shaders.size();
	m_shaders.push_back(shader); 
}



// Creates a frame buffer. Returns true if the buffer was set up.
// Returns false if the setup failed.

bool MainEngine::CreateFrameBuffer(int nWidth, int nHeight, FramebufferDesc& framebufferDesc, bool resolveOnly)
{
	framebufferDesc.m_size.x = nWidth;
	framebufferDesc.m_size.y = nHeight;

	if (!resolveOnly)
	{
		glGenFramebuffers(1, &framebufferDesc.m_nRenderFramebufferId);
		glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.m_nRenderFramebufferId);

		glGenRenderbuffers(1, &framebufferDesc.m_nDepthBufferId);
		glBindRenderbuffer(GL_RENDERBUFFER, framebufferDesc.m_nDepthBufferId);
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, nWidth, nHeight);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, framebufferDesc.m_nDepthBufferId);

		glGenTextures(1, &framebufferDesc.m_nRenderTextureId);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.m_nRenderTextureId);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, nWidth, nHeight, true);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.m_nRenderTextureId, 0);
	}

	glGenFramebuffers(1, &framebufferDesc.m_nResolveFramebufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.m_nResolveFramebufferId);

	glGenTextures(1, &framebufferDesc.m_nResolveTextureId);
	glBindTexture(GL_TEXTURE_2D, framebufferDesc.m_nResolveTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferDesc.m_nResolveTextureId, 0);

	GLErrorCheck(__func__, __LINE__);

	// check FBO status
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		printf("Framebuffer failed %d\n", status);
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return true;
}

bool MainEngine::SetupStereoRenderTargets()
{
	if (!NO_VR)
	{
		if (!m_pHMD)
			return false;
	}

	glGenBuffersARB(2, m_glDecoderPBO);
	glGenTextures(2, m_glDecoderTexture);

	glm::ivec2 decoderSize;
	decoderSize.x = m_width;
	decoderSize.y  = m_height;

	if (USE_COMPRESSOR_MAP)
	{
		decoderSize = ResolutionSettings::Get().Resolution(FRAME_BUFFERS);
	}

	int nImageWidth, nImageHeight, channels;

	unsigned char* imageRGBA = stbi_load("../content/encoded.png", &nImageWidth, &nImageHeight, &channels, STBI_rgb_alpha);

	if (imageRGBA == 0)
	{
		printf("test load error! %s\n", "../content/textures/alphabet.png");
		return false;
	}

	for (int i = 0; i < 2; i++)
	{
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, m_glDecoderPBO[i]);
		glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, decoderSize.x * decoderSize.y * 4ULL, NULL, GL_STREAM_DRAW_ARB);
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

		glBindTexture(GL_TEXTURE_RECTANGLE, m_glDecoderTexture[i]);
		glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA8, decoderSize.x, decoderSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_RECTANGLE, 0);
	}

	stbi_image_free(imageRGBA);

	int fbWidth = m_width;
	int fbHeight = m_height;

	//if (USE_SUPER_RESOLUTION)
	//{
	//	m_widthSR = m_width * 2;
	//	m_heightSR = m_height * 2;

	//	fbWidth = m_widthSR;
	//	fbHeight = m_heightSR;

	//	m_superScaleImageMem = (unsigned char*)malloc(fbWidth * fbHeight * 4);
	//}

	CreateFrameBuffer(fbWidth, fbHeight, m_leftEye, true);
	CreateFrameBuffer(fbWidth, fbHeight, m_rightEye, true);

	if(StatCalc::GetParameter("NVDEC") > 0)
	{
		for (int i = 0; i < 2; i++)
		{
			m_decoder[i] = new DecLowLatency();
		}

		CudaErr(cuInit(0));
		int iGpu = 0;
		CudaErr(cuDeviceGet(&m_cuDevice, iGpu));
		char szDeviceName[80];
		CudaErr(cuDeviceGetName(szDeviceName, sizeof(szDeviceName), m_cuDevice));
		std::cout << "GPU in use: " << szDeviceName << std::endl;

		for (int i = 0; i < 2; i++)
		{
			CudaErr(cuCtxCreate(&m_cuDecContext[i], CU_CTX_SCHED_BLOCKING_SYNC, m_cuDevice));
		}

		CUDA_ERROR_CHECK

			for (int i = 0; i < 2; i++)
			{
				CudaErr(cuCtxSetCurrent(m_cuDecContext[i]));
				CudaErr(cuGraphicsGLRegisterBuffer(&m_cuDecResource[i], m_glDecoderPBO[i], CU_GRAPHICS_REGISTER_FLAGS_WRITE_DISCARD));
			}
	}
	else
	{
		printf("Using Open H264 software decoding\n");

		for (int i = 0; i < 2; i++)
		{
			m_decoder[i] = new DecOpenH264();
		}
	}

	CreateDecoder();

	//MSE Calc
//CreateFrameBuffer(m_width, m_height, m_copyBuffer, true);

	return true;
}

void MainEngine::CreateDecoder()
{
	int resBase = 0;
	if (USE_COMPRESSOR_MAP)
	{
		resBase = FRAME_BUFFERS;
	}
	for (int i = 0; i < 2; i++)
	{
		if (StatCalc::GetParameter("NVDEC") > 0)
		{
			((DecLowLatency*)m_decoder[i])->InitDecoder(m_cuDecContext[i], i, &m_client, ResolutionSettings::Get().Resolution(resBase));
			CUDA_ERROR_CHECK
		}
		else
		{
			((DecOpenH264*)m_decoder[i])->InitDecoder(i, &m_client, ResolutionSettings::Get().Resolution(resBase));
		}
	}
}

void MainEngine::SetupCompanionWindow()
{
	if (!NO_VR)
	{
		if (!m_pHMD)
			return;
	}

	struct VertexDataWindow
	{
		glm::vec2 position;
		glm::vec2 texCoord;

		VertexDataWindow(const glm::vec2& pos, const glm::vec2 tex) : position(pos), texCoord(tex) {	}
	};

	std::vector<VertexDataWindow> vVerts;

	// left eye verts
	vVerts.push_back(VertexDataWindow(glm::vec2(-1, -1), glm::vec2(0, 1)));
	vVerts.push_back(VertexDataWindow(glm::vec2(0, -1), glm::vec2(1, 1)));
	vVerts.push_back(VertexDataWindow(glm::vec2(-1, 1), glm::vec2(0, 0)));
	vVerts.push_back(VertexDataWindow(glm::vec2(0, 1), glm::vec2(1, 0)));

	// right eye verts
	vVerts.push_back(VertexDataWindow(glm::vec2(0, -1), glm::vec2(0, 1)));
	vVerts.push_back(VertexDataWindow(glm::vec2(1, -1), glm::vec2(1, 1)));
	vVerts.push_back(VertexDataWindow(glm::vec2(0, 1), glm::vec2(0, 0)));
	vVerts.push_back(VertexDataWindow(glm::vec2(1, 1), glm::vec2(1, 0)));

	GLushort vIndices[] = { 0, 1, 3,   0, 3, 2,   4, 5, 7,   4, 7, 6 };
	m_uiCompanionWindowIndexSize = 12;

	glGenVertexArrays(1, &m_unCompanionWindowVAO);
	glBindVertexArray(m_unCompanionWindowVAO);

	glGenBuffers(1, &m_glCompanionWindowIDVertBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_glCompanionWindowIDVertBuffer);
	glBufferData(GL_ARRAY_BUFFER, vVerts.size() * sizeof(VertexDataWindow), &vVerts[0], GL_STATIC_DRAW);

	glGenBuffers(1, &m_glCompanionWindowIDIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glCompanionWindowIDIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_uiCompanionWindowIndexSize * sizeof(GLushort), &vIndices[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataWindow), (void*)offsetof(VertexDataWindow, position));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataWindow), (void*)offsetof(VertexDataWindow, texCoord));


	//glDisableVertexAttribArray(0);
	//glDisableVertexAttribArray(1);
	glBindVertexArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void MainEngine::GLErrorCheck(const char* _method, int _line)
{
#ifdef DO_GL_ERROR_CHECKS
	GLenum err = glGetError();
	if (err != GL_NO_ERROR)
		printf("%d %s %d\n", _line, _method, err);
#endif
}



// 

void MainEngine::RenderCompanionWindow()
{
	//glDisable(GL_DEPTH_TEST);
	glViewport(0, 0, m_nCompanionWindowWidth, m_nCompanionWindowHeight);

	glBindVertexArray(m_unCompanionWindowVAO);
	m_shaders[m_shaderCompanion]->UseProgram();

	// render left eye (first half of index array )
	glBindTexture(GL_TEXTURE_2D, m_leftEye.m_nResolveTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glDrawElements(GL_TRIANGLES, m_uiCompanionWindowIndexSize / 2, GL_UNSIGNED_SHORT, 0);

	// render right eye (second half of index array )
	glBindTexture(GL_TEXTURE_2D, m_rightEye.m_nResolveTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glDrawElements(GL_TRIANGLES, m_uiCompanionWindowIndexSize / 2, GL_UNSIGNED_SHORT, (const void*)(uintptr_t)(m_uiCompanionWindowIndexSize));

	glBindVertexArray(0);
	glUseProgram(0);
}

// ---


//Client only


// ---

// This function will poll the distortion data from the HMD and send it to server with TCP, it will also save the data to disk.
// The data is saved to disk because then it can be used later if running without an HMD. 
// We could do this once and always use the same lens file, but it does it every startup for now.

void MainEngine::SetupDistortion()
{
	bool fromFile = false;
	if (NO_VR)
	{
		fromFile = true;
	}
	std::vector<VertexDataLens> vVerts(0);

	if (fromFile == false)
	{
		if (!m_pHMD)
		{
			printf("NO DISTORTION LENS!!\n");
			return;
		}

		GLushort m_iLensGridSegmentCountH = 43;
		GLushort m_iLensGridSegmentCountV = 43;

		float w = (float)(1.0 / float(m_iLensGridSegmentCountH - 1));
		float h = (float)(1.0 / float(m_iLensGridSegmentCountV - 1));

		float u, v = 0;

		VertexDataLens vert;
		vr::DistortionCoordinates_t dc0;

		//left eye distortion verts
		float Xoffset = -1;
		for (int y = 0; y < m_iLensGridSegmentCountV; y++)
		{
			for (int x = 0; x < m_iLensGridSegmentCountH; x++)
			{
				u = x * w;
				v = 1 - y * h;
				vert.position = glm::vec2(Xoffset + u * 2, -1 + 2 * y * h);

				//printf("%.2f, %.2f\n", vert.position.x, vert.position.y);

				if (m_pHMD->ComputeDistortion(vr::Eye_Left, u, v, &dc0))
				{
					vert.texCoordRed = glm::vec2(dc0.rfRed[0], 1 - dc0.rfRed[1]);
					vert.texCoordGreen = glm::vec2(dc0.rfGreen[0], 1 - dc0.rfGreen[1]);
					vert.texCoordBlue = glm::vec2(dc0.rfBlue[0], 1 - dc0.rfBlue[1]);

					vVerts.push_back(vert);
				}
				else
				{
					printf("Distortion error\n");
					system("pause");
				}
			}
		}

		//right eye distortion verts
		Xoffset = -1;
		for (int y = 0; y < m_iLensGridSegmentCountV; y++)
		{
			for (int x = 0; x < m_iLensGridSegmentCountH; x++)
			{
				u = x * w;
				v = 1 - y * h;
				vert.position = glm::vec2(Xoffset + u * 2, -1 + 2 * y * h);

				if (m_pHMD->ComputeDistortion(vr::Eye_Right, u, v, &dc0))
				{

					vert.texCoordRed = glm::vec2(dc0.rfRed[0], 1 - dc0.rfRed[1]);
					vert.texCoordGreen = glm::vec2(dc0.rfGreen[0], 1 - dc0.rfGreen[1]);
					vert.texCoordBlue = glm::vec2(dc0.rfBlue[0], 1 - dc0.rfBlue[1]);

					vVerts.push_back(vert);
				}
				else
				{
					printf("Distortion error\n");
					system("pause");
				}
			}
		}

	}
	else
	{
		std::ifstream lensData;
		lensData.open("../content/lensData", std::ifstream::binary | std::ifstream::in);

		if (!lensData.good())
		{
			printf("Failed to open lens data file!\n");
			system("pause");
		}

		while (!lensData.eof())
		{
			VertexDataLens vertData;
			lensData.read((char*)&vertData, sizeof(VertexDataLens));
			vVerts.push_back(vertData);
		}
	}

	printf("Read %llu lens vertices\n", vVerts.size());

	Pkt::Cl_LensData pkt;
	int index = 0;

	while (index < vVerts.size())
	{
		//Timestamp is size
		pkt.TimeStamp = 0;

		for (int i = 0; i < 64; i++)
		{
			if (index >= vVerts.size())
				break;

			pkt.TimeStamp++;

			pkt.Data[i] = vVerts[index];
			index++;
		}

		if (index >= vVerts.size())
		{
			pkt.IsLastPkt = true;
		}

		m_client.MessageTCP(sizeof(Pkt::RVR_Header) + pkt.TimeStamp * sizeof(VertexDataLens), (char*)&pkt, 0);
	}

	std::ofstream lensFile;

	lensFile.open("../content/lensData", std::ofstream::binary | std::ofstream::out);

	if (lensFile.good())
	{
		lensFile.write((char*)vVerts.data(), vVerts.size() * sizeof(VertexDataLens));

		lensFile.close();
	}
	else
	{
		printf("Failed to save lens data!!!\n");
	}
}

void MainEngine::WriteHiddenAreaMesh(const char* _path, const vr::HiddenAreaMesh_t& _mesh)
{
	FILE* meshFile;

	fopen_s(&meshFile, _path, "rb");

	if (meshFile == NULL)
	{
		fopen_s(&meshFile, _path, "wb");
		if (meshFile != NULL)
		{
			fwrite(&_mesh.unTriangleCount, sizeof(uint32_t), 1, meshFile);
			size_t result = fwrite(_mesh.pVertexData, sizeof(vr::HmdVector2_t), _mesh.unTriangleCount * 3ULL, meshFile);
			fclose(meshFile);

			printf("Hidden area mesh written to disk, %zd %s\n", result, _path);
		}
		else
		{
			printf("Failed to write hidden area mesh to disk! %s\n", _path);
		}
	}
	else
	{
		printf("Hidden area mesh found on disk, no write: %s\n",_path);
		fclose(meshFile);
	}
}

void MainEngine::ReadHiddenAreaMesh(const char* _path, vr::HiddenAreaMesh_t& _mesh)
{
	FILE* meshFile;

	fopen_s(&meshFile, _path, "rb");

	if (meshFile == NULL)
	{
		printf("Failed to read hidden area mesh from disk! %s\n", _path);
	}
	else
	{
		fread(&_mesh.unTriangleCount, sizeof(uint32_t), 1, meshFile);

		_mesh.pVertexData = (vr::HmdVector2_t*)malloc(sizeof(vr::HmdVector2_t) * _mesh.unTriangleCount * 3ULL);

		fread((void*)_mesh.pVertexData, sizeof(vr::HmdVector2_t), _mesh.unTriangleCount * 3ULL, meshFile);
		printf("Hidden area mesh found on disk, %d triangles %s\n", _mesh.unTriangleCount, _path);
		fclose(meshFile);
	}
}

void MainEngine::SendHiddenAreaMesh()
{
	vr::HiddenAreaMesh_t stencilMeshL;
	vr::HiddenAreaMesh_t stencilMeshR;

	if (!NO_VR)
	{
		stencilMeshL = vr::VRSystem()->GetHiddenAreaMesh(vr::EVREye::Eye_Left);
		stencilMeshR = vr::VRSystem()->GetHiddenAreaMesh(vr::EVREye::Eye_Right);

		WriteHiddenAreaMesh("../content/HiddenAreaMesh_Left", stencilMeshL);
		WriteHiddenAreaMesh("../content/HiddenAreaMesh_Right", stencilMeshR);
	}
	else
	{
		ReadHiddenAreaMesh("../content/HiddenAreaMesh_Left", stencilMeshL);
		ReadHiddenAreaMesh("../content/HiddenAreaMesh_Right", stencilMeshR);
	}

	Pkt::Cl_HiddenAreaMesh meshPkt;

	//LEFT EYE
	meshPkt.TimeStamp = stencilMeshL.unTriangleCount;
	meshPkt.IsLastPkt = 0; //Left eye
	m_client.MessageTCP(sizeof(Pkt::Cl_HiddenAreaMesh), &meshPkt, 0);
	m_client.MessageTCP(sizeof(vr::HmdVector2_t) * stencilMeshL.unTriangleCount * 3, (void*)stencilMeshL.pVertexData, 0);
	printf("Sent hidden area mesh of %d triangles per eye\n", stencilMeshL.unTriangleCount);

	//RIGHT EYE
	meshPkt.TimeStamp = stencilMeshR.unTriangleCount;
	meshPkt.IsLastPkt = 1; //Right eye
	m_client.MessageTCP(sizeof(Pkt::Cl_HiddenAreaMesh), &meshPkt, 0);
	m_client.MessageTCP(sizeof(vr::HmdVector2_t) * stencilMeshR.unTriangleCount * 3, (void*)stencilMeshR.pVertexData, 0);

	//If there is no VR, then these were read from disk and should be freed here, otherwise they're in steam
	if (NO_VR)
	{
		free((void*)stencilMeshL.pVertexData);
		free((void*)stencilMeshR.pVertexData);
	}

	return;
}


// Initialize Compositor. Returns true if the compositor was
// successfully initialized, false otherwise.

bool MainEngine::InitCompositor()
{
	vr::EVRInitError peError = vr::VRInitError_None;

	if (!vr::VRCompositor())
	{
		printf("Compositor initialization failed. See log file for details\n");
		return false;
	}

	return true;
}


bool MainEngine::RenderCudaToGL()
{
	//glDisable(GL_DEPTH_TEST);

	//Will render to native size
	const glm::ivec2& nativeSize = ResolutionSettings::Get().Resolution(m_client.GetCurrentLevel());

	glViewport(0, 0, nativeSize.x, nativeSize.y);

	GLErrorCheck(__func__, __LINE__);

	//for (int _eye = 0; _eye < 2; _eye++)
	//{
	//	m_decoder[_eye]->GoProcess();
	//}

	if(StatCalc::GetParameter("NVDEC") > 0)
	{
		for (int i = 0; i < 2; i++)
		{
			if (CopyImageFromDecoder(i) == false)
			{
				return false;
			}
		}
	}
	else
	{
		for (int i = 0; i < 2; i++)
		{
			if (CopyImageFromDecoderOpenH264(i) == false)
			{
				return false;
			}
		}
	}

	//glFlush();
	//glFinish();

	PostDecodeDelayDecision();

	GLErrorCheck(__func__, __LINE__);

	if (USE_SUPER_RESOLUTION)
	{
		//From decoder texture to RAM
		//glBindTexture(GL_TEXTURE_RECTANGLE, m_glDecoderTexture[0]);
		//glGetTexImage(GL_TEXTURE_RECTANGLE, 0, GL_RGB, GL_UNSIGNED_BYTE, m_frameSavingData[0]);
		//glBindTexture(GL_TEXTURE_RECTANGLE, 0);

		//glBindTexture(GL_TEXTURE_RECTANGLE, m_glDecoderTexture[1]);
		//glGetTexImage(GL_TEXTURE_RECTANGLE, 0, GL_RGB, GL_UNSIGNED_BYTE, m_frameSavingData[1]);
		//glBindTexture(GL_TEXTURE_RECTANGLE, 0);

		//From RAM to upscale OpenCV
		//for (int i = 0; i < 2; i++)
		//{
		//	FramebufferDesc& eye = m_leftEye;
		//	if (i > 0)
		//		eye = m_rightEye;

		//	cv::Mat m_superResMat;
		//	//m_superScale->ScaleMeVRAM(m_frameSavingData[i], m_superResMat, m_decoder[i]->GetWidth(), m_decoder[i]->GetHeight());
		//	m_superScale->ScaleMePNG("../QOE/SuperRes/Test1.png", m_superResMat);
		//	//memcpy(m_superScaleImageMem, m_superResMat.data, m_widthSR*m_heightSR * 3);

		//	//From OpenCV to OpenGL
		//	glBindTexture(GL_TEXTURE_2D, eye.m_nResolveTextureId);
		//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, m_widthSR, m_heightSR, 0, GL_RGB, GL_UNSIGNED_BYTE, m_superResMat.data);
		//	glBindTexture(GL_TEXTURE_2D, 0);
		//}
	}
	else
	{
		DecodedTextureResize();
	}

	//glFlush();
	//glFinish();

	//GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	//glWaitSync(sync, 0, GL_TIMEOUT_IGNORED);

	GLErrorCheck(__func__, __LINE__);

	return true;
}

bool MainEngine::CopyImageFromDecoder(int _eye)
{
	glm::ivec2 nativeSize = ResolutionSettings::Get().Resolution(0);

	if (USE_COMPRESSOR_MAP)
	{
		nativeSize = ResolutionSettings::Get().Resolution(COMPRESSOR_BUFFER_INDEX);
	}

	CUdeviceptr devicePtr = ((DecLowLatency*)m_decoder[_eye])->GetDeviceFramePtr();

	//Safety
	if (devicePtr == 0)
		return false;

	if (m_cuDecResource[_eye] == 0)
		return false;

	//Are we allowed to pick a frame from the decoder? Here we can allow more frame lag to impose a delay buffer
	if (m_decoder[_eye]->WaitCurrentFrameReady(m_predictFramesAhead, m_clFrameIndex[_eye] + (1 + m_predictionsKickedOff)) == false)
		return false;

	m_clFrameIndex[_eye]++;

	//printf("Sending frame to cuda (Eye %d) (%d)\n",_eye, m_clFrameIndex[_eye]);

	CudaErr(cuCtxSetCurrent(m_cuDecContext[_eye]));

	//Cuda copy

	CudaErr(cuGraphicsMapResources(1, &m_cuDecResource[_eye], 0));
	CUdeviceptr dpBackBuffer;
	size_t nSize = 0;
	CudaErr(cuGraphicsResourceGetMappedPointer(&dpBackBuffer, &nSize, m_cuDecResource[_eye]));

	CUDA_MEMCPY2D cudaMemCpyData = { 0 };

	cudaMemCpyData.srcMemoryType = CU_MEMORYTYPE_DEVICE;
	cudaMemCpyData.srcDevice = devicePtr;
	cudaMemCpyData.srcPitch = m_decoder[_eye]->GetWidth() * 4;
	cudaMemCpyData.dstMemoryType = CU_MEMORYTYPE_DEVICE;
	cudaMemCpyData.dstDevice = dpBackBuffer;
	cudaMemCpyData.dstPitch = nSize / m_decoder[_eye]->GetHeight();
	cudaMemCpyData.WidthInBytes = m_decoder[_eye]->GetWidth() * 4;
	cudaMemCpyData.Height = m_decoder[_eye]->GetHeight();

	//CudaErr(cuMemcpy2DAsync(&cudaMemCpyData, 0));
	CudaErr(cuMemcpy2D(&cudaMemCpyData));

	CudaErr(cuGraphicsUnmapResources(1, &m_cuDecResource[_eye], 0));

	//Copy pixel buffer to texture rectangle
	glBindTexture(GL_TEXTURE_RECTANGLE, m_glDecoderTexture[_eye]);
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, m_glDecoderPBO[_eye]);
	glTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, nativeSize.x, nativeSize.y, GL_BGRA, GL_UNSIGNED_BYTE, 0);

	//Unbind
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
	glBindTexture(GL_TEXTURE_RECTANGLE, 0);

	GLErrorCheck(__func__, __LINE__);

	return true;
}

bool MainEngine::CopyImageFromDecoderOpenH264(int _eye)
{
	glm::ivec2 nativeSize = ResolutionSettings::Get().Resolution(0);

	if (USE_COMPRESSOR_MAP)
	{
		nativeSize = ResolutionSettings::Get().Resolution(COMPRESSOR_BUFFER_INDEX);
	}

	//Are we allowed to pick a frame from the decoder? Here we can allow more frame lag to impose a delay buffer
	//if (m_predictFramesAhead > 0)
	//{
	//	int sends = m_predictFramesAhead - m_predictionsKickedOff;
	//	for (int i = 0; i < sends;i++)
	//	{
	//		TransmitInputMatrix(1.0f / 90.0f, true);
	//		m_predictionsKickedOff++;
	//		printf("Kicking off delay\n");
	//	}
	//}

	if (m_decoder[_eye]->WaitCurrentFrameReady(m_predictFramesAhead, m_clFrameIndex[_eye] + (1)) == false)
		return false;

	unsigned char* rgbData = ((DecOpenH264*)m_decoder[_eye])->GetRgbPtr();

	m_clFrameIndex[_eye]++;

	//printf("Sending frame to cuda (Eye %d) (%d)\n",_eye, m_clFrameIndex[_eye]);

	// bind PBO to update texture source
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, m_glDecoderPBO[_eye]);

	// Note that glMapBuffer() causes sync issue.
	// If GPU is working with this buffer, glMapBuffer() will wait(stall)
	// until GPU to finish its job. To avoid waiting (idle), you can call
	// first glBufferData() with NULL pointer before glMapBuffer().
	// If you do that, the previous data in PBO will be discarded and
	// glMapBuffer() returns a new allocated pointer immediately
	// even if GPU is still working with the previous data.

	size_t size = m_decoder[_eye]->GetWidth() * m_decoder[_eye]->GetHeight() * 4;

	glBufferData(GL_PIXEL_UNPACK_BUFFER, size, 0, GL_STREAM_DRAW);

	// map the buffer object into client's memory
	GLubyte* ptr = (GLubyte*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
	if (ptr)
	{
		// update data directly on the mapped buffer
		memcpy(ptr, rgbData, size);
		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); // release the mapped buffer
	}
	else
	{
		printf("glMapBuffer failed ! %d\n", glGetError());
	}

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	//Copy pixel buffer to texture rectangle
	glBindTexture(GL_TEXTURE_RECTANGLE, m_glDecoderTexture[_eye]);
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, m_glDecoderPBO[_eye]);

	glTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, nativeSize.x, nativeSize.y, GL_BGRA, GL_UNSIGNED_BYTE, 0);

	//Unbind
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
	glBindTexture(GL_TEXTURE_RECTANGLE, 0);

	GLErrorCheck(__func__, __LINE__);

	return true;
}

void MainEngine::ComputeDecompression(GLuint _decoderTexture, GLuint _resolveTexture, GLuint _compressionTexture)
{
	m_shaders[m_shaderDecompress]->UseProgram();

	const glm::ivec2& res = ResolutionSettings::Get().Resolution(COMPRESSOR_BUFFER_INDEX);

	int width = res.x / 16;
	int height = res.y / 16;

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_RECTANGLE, _decoderTexture ); //m_glDecoderTexture[0]

	glBindImageTexture(1, _resolveTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8); // m_leftEye.m_nResolveTextureId
	glBindImageTexture(2, _compressionTexture , 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG16UI); //m_compressorMap.GetCompressionTexture(0)

	m_shaders[m_shaderDecompress]->SetUniformV_loc("gEncWidth", res.x);
	m_shaders[m_shaderDecompress]->SetUniformV_loc("gEncHeight", res.y);

	glDispatchCompute(width, height, 1);

	glBindTexture(GL_TEXTURE_RECTANGLE, 0);
}


void MainEngine::DecodedTextureResize()
{
	glBeginQuery(GL_TIME_ELAPSED, m_timeQuery);

	if (USE_COMPRESSOR_MAP)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_leftEye.m_nResolveFramebufferId);
		glClear(GL_COLOR_BUFFER_BIT);
		ComputeDecompression(m_glDecoderTexture[0], m_leftEye.m_nResolveTextureId, m_compressorMap.GetCompressionTexture(0));

		glBindFramebuffer(GL_FRAMEBUFFER, m_rightEye.m_nResolveFramebufferId);
		glClear(GL_COLOR_BUFFER_BIT);
		ComputeDecompression(m_glDecoderTexture[1], m_rightEye.m_nResolveTextureId, m_compressorMap.GetCompressionTexture(1));
	}
	else
	{
		//Render to 2D quads, one per eye
		glBindVertexArray(m_2DVAO);

		m_shaders[m_shaderQuadDec]->UseProgram();

		//Draw left eye
		glBindFramebuffer(GL_FRAMEBUFFER, m_leftEye.m_nResolveFramebufferId);
		glBindTexture(GL_TEXTURE_RECTANGLE, m_glDecoderTexture[0]);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		//Draw right eye
		glBindFramebuffer(GL_FRAMEBUFFER, m_rightEye.m_nResolveFramebufferId);
		glBindTexture(GL_TEXTURE_RECTANGLE, m_glDecoderTexture[1]);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		//Unbind
		glBindVertexArray(0);
	}

	glEndQuery(GL_TIME_ELAPSED);

	glGetQueryObjectui64v(m_timeQuery, GL_QUERY_RESULT, &m_displayTime);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void MainEngine::PostDecodeDelayDecision()
{
	//Doesnt matter
	//
	//static int desyncs = 0;
	//if (m_decoder[0]->GetLastFrameID() != m_decoder[1]->GetLastFrameID())
	//{
	//	//Images are not on the same index, this will get fixed if we wait up
	//	printf("Images have different index! %d %d\n", m_decoder[0]->GetLastFrameID(), m_decoder[1]->GetLastFrameID());

	//	//RenderCudaToGL();

	//	//m_kickOffDelay = true;

	//	if (m_predictFramesAhead == 0)
	//	{
	//		printf("Avoiding desync... (Happens every start)\n");

	//		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	//		desyncs++;

	//		if (desyncs > 3)
	//		{
	//			//Skip frame if things take too long, this was used before but shouldnt happen any longer

	//			if (m_decoder[0]->GetLastFrameID() > m_decoder[1]->GetLastFrameID())
	//			{
	//				m_decoder[1]->ArtificialIncreaseFrameID();
	//			}
	//			else
	//			{
	//				m_decoder[0]->ArtificialIncreaseFrameID();
	//			}

	//			printf("Frame was skipped!\n");
	//		}
	//	}
	//}

	if (m_kickOffDelay)
	{
		// Experimental delay function, this will start a new rendering at the server immediately while the client will still have to wait for new poses.
		// So two frames will be in the pipeline, but you must also enable frames to be buffered in the decoder or it will stop this. 
		// Set m_predictFramesAhead to the number of frames delayed to use this

		m_kickOffDelay = false;
		TransmitInputMatrix(1.0f / 90.0f, true);
		printf("kicking off delay!\n");
		m_predictionsKickedOff++;
		Sleep(11);
	}
}

//Computes mean square error MSE for special experiments, this shows the difference between this and prev. image
void MainEngine::ComputeMSE(bool _freeze)
{
	//return;
	m_shaders[m_shaderMSE]->UseProgram();
	m_shaders[m_shaderMSE]->SetUniformV_loc("width", (int)m_width);
	m_shaders[m_shaderMSE]->SetUniformV_loc("height", (int)m_height);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_copyBuffer.m_nResolveTextureId);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_rightEye.m_nResolveTextureId);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_MSEBuffer);
	glDispatchCompute(32, 32, 1);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	glUseProgram(0);
	glActiveTexture(GL_TEXTURE0);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_rightEye.m_nResolveFramebufferId);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_copyBuffer.m_nResolveFramebufferId);

	glBlitFramebuffer(0, 0, m_width, m_height, 0, 0, m_width, m_height, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	double MSE = 0;
	double* ptr = (double*)m_MSEBufferPtr;
	for (int i = 0; i < m_MSEBufferSize; i++)
	{
		MSE += ptr[i];
	}

	MSE /= (m_MSEBufferSize);

	m_lastMSE = MSE;

	GLErrorCheck(__func__, __LINE__);
}


// Processes a single VR event

void MainEngine::ProcessVREvent(const vr::VREvent_t& event)
{
	switch (event.eventType)
	{
	case vr::VREvent_TrackedDeviceDeactivated:
	{
		printf("Device %u detached.\n", event.trackedDeviceIndex);
	}
	break;
	case vr::VREvent_TrackedDeviceUpdated:
	{
		printf("Device %u updated.\n", event.trackedDeviceIndex);
	}
	break;
	}
}

void MainEngine::TransmitInputMatrix(float _dt, bool _force)
{

	if (m_client.IsConnected())
	{
		//Creates a reference at whatever number the decoder is at now, to see if a frame was finished later.
		//for (int i = 0; i < 2; i++)
		//{
		//	m_decoder[i]->SetCurrentFrame();
		//}

		//for (int i = 0; i < 2; i++)
		//{
		//	m_decoder[i]->SetCurrentFrame();
		//}

		//long long t0 = Timers::Get().Time();



		//Input is sent to server
		m_client.SendInputMatrix(_force, _dt, m_predictFramesAhead);


		Timers& timer = Timers::Get();
		long long finish = Timers::Get().Time();
		timer.PushLongLong(1, T_DIS_FIN, m_clFrameIndex[1], finish);
		timer.PushLongLong(0, T_DIS_FIN, m_clFrameIndex[0], finish);

		for (int i = 0; i < 2; i++)
		{
			//m_decoder[i]->SetCurrentFrame();
			m_decoder[i]->GoProcess();
		}

		m_client.EnsureRetransmission();

		//for (int i = 0; i < 2; i++)
		//{
		//	m_decoder[i]->GoProcess();
		//}
	}
}


void MainEngine::ExperimentLogic()
{
	//This will dump 500 of the first frames to let encoder get into steady state
	if (!Timers::Get().HasStartedPrinting())
		return;

	if (SOAK_TEST || RESOLUTION_TEST || BITRATE_TEST || TWITCH_TEST || INCREASING_ROTATION_TEST)
	{
		static int framesRecordedAtThisLevel = 0;
		static int level = 0;

		framesRecordedAtThisLevel++;

		if (framesRecordedAtThisLevel >= NUM_FRAMES_TO_TEST)
		{
			level++;
			framesRecordedAtThisLevel = 0;

			if (SOAK_TEST)
			{
				PrintFrame(); //Prints at client
				m_client.SetKeyPressExtraHack(SDLK_6);//Quit
			}
			if (RESOLUTION_TEST)
			{
				m_client.SetKeyPressExtraHack(SDLK_1 + level); //Change resolution
			}
			if (BITRATE_TEST)
			{
				m_client.SetKeyPressExtraHack(SDLK_MINUS);//Decrease bitrate
				if (m_client.GetBitRate() <= 1000)
				{
					m_client.SetKeyPressExtraHack(SDLK_ESCAPE); //Quit
					m_quit = true;
				}
			}
			if (TWITCH_TEST)
			{
				m_client.SetKeyPressExtraHack(SDLK_ESCAPE); //Quit
				m_quit = true;
			}
			if (INCREASING_ROTATION_TEST)
			{
				double currentRotation = StatCalc::GetParameter("ROTATING_DEG_PER_SEC");
				currentRotation += StatCalc::GetParameter("ROTATION_INCREMENT");

				StatCalc::SetParameter("ROTATING_DEG_PER_SEC", currentRotation);

				printf("Speed is now %f\n",currentRotation);

				if (currentRotation > 1080)
				{
					m_client.SetKeyPressExtraHack(SDLK_ESCAPE); //Quit
					m_quit = true;
				}
			}
		}

		if (PRINT_EVERY_FRAME)
		{
			//PNG all the time
			PrintFrame(); //Prints at client
			m_client.SetKeyPress(SDLK_p); //Prints at server
		}

		if (BITRATE_TEST)
		{
			//One PNG in the middle test
			if (framesRecordedAtThisLevel == NUM_FRAMES_TO_TEST / 2)
			{
				PrintFrame(); //Prints at client
				m_client.SetKeyPress(SDLK_p); //Prints at server
			}
		}

		if (TWITCH_TEST)
		{
			if (framesRecordedAtThisLevel == 20)
			{
				m_VRInputs->SetRotateViewOnOff(true);
			}

			if (framesRecordedAtThisLevel == 40)
			{
				m_VRInputs->SetRotateViewOnOff(false);
			}
		}
	}

	if (ROTATION_FROM_FILE_TEST)
	{
		if (m_VRInputs->IsRotationTestComplete())
		{
			m_client.SetKeyPressExtraHack(SDLK_ESCAPE); //Quit
			m_quit = true;
		}
	}

	if (LOSS_TEST)
	{
		static bool once = 0;

		if (!once)
		{
			m_client.SetKeyPress(SDLK_y);
			m_specialTestButton = !m_specialTestButton;
			printf("Loss test started %d\n", m_specialTestButton);
			once = true;
		}
		else
		{
			m_client.SetKeyPress(0);
		}
	}

	
}

void MainEngine::SendHeadsetInfo()
{
	SendHiddenAreaMesh();

	Pkt::Cl_Input inputPacket;

	//Projections and positions of eyes in this specific headset

	inputPacket.InputType = Pkt::InputTypes::MATRIX_EYE_POS_LEFT;
	memcpy(inputPacket.Data, &m_VRInputs->GetHMDMatrixPoseEye(vr::Eye_Left), sizeof(glm::mat4));
	m_client.MessageTCP(sizeof(Pkt::Cl_Input), (char*)&inputPacket, 0);

	inputPacket.InputType = Pkt::InputTypes::MATRIX_EYE_POS_RIGHT;
	memcpy(inputPacket.Data, &m_VRInputs->GetHMDMatrixPoseEye(vr::Eye_Right), sizeof(glm::mat4));
	m_client.MessageTCP(sizeof(Pkt::Cl_Input), (char*)&inputPacket, 0);

	inputPacket.InputType = Pkt::InputTypes::MATRIX_EYE_PROJ_LEFT;
	memcpy(inputPacket.Data, &m_VRInputs->GetHMDMatrixProjectionEye(vr::Eye_Left, m_width, m_height), sizeof(glm::mat4));
	m_client.MessageTCP(sizeof(Pkt::Cl_Input), (char*)&inputPacket, 0);

	inputPacket.InputType = Pkt::InputTypes::MATRIX_EYE_PROJ_RIGHT;
	memcpy(inputPacket.Data, &m_VRInputs->GetHMDMatrixProjectionEye(vr::Eye_Right, m_width, m_height), sizeof(glm::mat4));
	m_client.MessageTCP(sizeof(Pkt::Cl_Input), (char*)&inputPacket, 0);

	//All desired resolutions, also sets the framerate of each resolution, which is the same for all, but could be changed for some experiment
	for (int i = 0; i < 5; i++)
	{
		inputPacket.InputType = Pkt::InputTypes(Pkt::InputTypes::VECTOR_RESOLUTION_0 + i);
		inputPacket.Misc = (unsigned char)StatCalc::GetParameter("FRAME_RATE");
		memcpy(inputPacket.Data, &ResolutionSettings::Get().Resolution(i), sizeof(glm::ivec2));
		m_client.MessageTCP(sizeof(Pkt::Cl_Input), (char*)&inputPacket, 0);
	}
}

void MainEngine::CreateQuadVBO()
{
	float positionData[] = {
		-1.0, -1.0,
		-1.0, 1.0,
		1.0, -1.0,
		-1.0, 1.0,
		1.0, 1.0,
		1.0, -1.0 };

	float texCoordData[] = {
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f };

	int nrOfPoints = 12;

	glGenBuffers(2, m_2DVBO);

	// "Bind" (switch focus to) first buffer
	glBindBuffer(GL_ARRAY_BUFFER, m_2DVBO[0]);
	glBufferData(GL_ARRAY_BUFFER, nrOfPoints * sizeof(float), positionData, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, m_2DVBO[1]);
	glBufferData(GL_ARRAY_BUFFER, nrOfPoints * sizeof(float), texCoordData, GL_STATIC_DRAW);

	// create 1 VAO
	glGenVertexArrays(1, &m_2DVAO);
	glBindVertexArray(m_2DVAO);

	// enable "vertex attribute arrays"
	glEnableVertexAttribArray(0); // position
	glEnableVertexAttribArray(1); // texCoord

								  // map index 0 to position buffer
	glBindBuffer(GL_ARRAY_BUFFER, m_2DVBO[0]);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (GLubyte*)NULL);

	glBindBuffer(GL_ARRAY_BUFFER, m_2DVBO[1]);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (GLubyte*)NULL);

	glBindVertexArray(0); // disable VAO
	glUseProgram(0); // disable shader program
}