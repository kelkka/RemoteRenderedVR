#include "MainEngineGL.h"
#include "PNG.h"


MainEngineGL::MainEngineGL(int argc, char *argv[])
{
#ifdef MEASURE_CUDA_MEMCPY
	m_fileOutput.open("../content/output/memcpyTimes.txt", std::ofstream::out);
#endif
}

MainEngineGL::~MainEngineGL()
{
	printf("Shutdown");
}

bool MainEngineGL::Init()
{
	printf("USING OPENGL\n\n");
	m_VRInputs = new VRInputs();
	m_server = new Server(m_VRInputs);

	MOVING_SCENE = StatCalc::GetParameter("MOVING_SCENE") > 0;
	NO_LENS_DISTORTION = StatCalc::GetParameter("NO_LENS_DISTORTION") > 0;
	COMPANION_WINDOW = StatCalc::GetParameter("COMPANION_WINDOW") > 0;
	FIXED_FRAME_RATE = StatCalc::GetParameter("FIXED_FRAME_RATE") > 0;
	USE_COMPRESSOR_MAP = StatCalc::GetParameter("USE_COMPRESSOR_MAP") > 0;

	
	if (m_server->InitNetwork() < 0)
		return false;

	WaitForResolution();

	int nWindowPosX = 700;
	int nWindowPosY = 100;
	Uint32 unWindowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
	//SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY );
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
	if (m_bDebugOpenGL)
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

	SetProcessDPIAware();

	const char* name = "Server";

	m_nCompanionWindowHeight = 1280;
	m_nCompanionWindowWidth = uint32_t(m_nCompanionWindowHeight * 0.9f);


	m_pCompanionWindow = SDL_CreateWindow(name, nWindowPosX, nWindowPosY, m_nCompanionWindowWidth, m_nCompanionWindowHeight, unWindowFlags);
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
	int vsync = 0;

	vsync = 0;

	if (SDL_GL_SetSwapInterval(vsync) < 0)
	{
		printf("%s - Warning: Unable to set VSync! SDL Error: %s\n", __FUNCTION__, SDL_GetError());
		//return false;
	}

	m_uiVertcount = 0;

	if (!InitGL())
	{
		printf("%s - Unable to initialize OpenGL!\n", __FUNCTION__);
		return false;
	}

	GLErrorCheck(__func__, __LINE__);

	m_textRenderer = TextRenderer::GetInstance();
	m_textRenderer.LoadText(m_width * 2, m_height);
	m_frameCounterString = m_textRenderer.AddString(new C_RenderString(glm::vec3(1, 1, 1), 4, glm::vec3(0.5f, 0.4f, 0.0f), 0, "Feedback", C_RenderString::Align::CENTER, 1, 0, glm::vec3(0), false));

	m_textRenderer.SetGroupVisibility(0, 1);


	return true;
}



//  Outputs the string in message to debugging output.
//          All other parameters are ignored.
//          Does not return any meaningful value or reference.

void APIENTRY DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam)
{
	printf("GL Error: %s\n", message);
}



//  Initialize OpenGL. Returns true if OpenGL has been successfully
//          initialized, false if shaders could not be created.
//          If failure occurred in a module other than shaders, the function
//          may return true or throw an error. 

bool MainEngineGL::InitGL()
{
	if (m_bDebugOpenGL)
	{
		glDebugMessageCallback((GLDEBUGPROC)DebugCallback, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	}

	if (USE_COMPRESSOR_MAP)
	{
		m_compressorMap.LoadMaps();
	}

	CreateAllShaders();

	TEXTURE_RED = LoadTexture("../content/textures/red.png", GL_REPEAT);
	TEXTURE_BLUE = LoadTexture("../content/textures/blue.png", GL_REPEAT);
	TEXTURE_GRAY = LoadTexture("../content/textures/gray.png", GL_REPEAT);
	TEXTURE_YELLOW = LoadTexture("../content/textures/yellow.png", GL_REPEAT);
	TEXTURE_CHECKER = LoadTexture("../content/textures/checker.png", GL_REPEAT);

	m_textures.push_back(TEXTURE_RED);
	m_textures.push_back(TEXTURE_BLUE);
	m_textures.push_back(TEXTURE_GRAY);
	m_textures.push_back(TEXTURE_YELLOW);
	m_textures.push_back(TEXTURE_CHECKER);


	SetupScene();

	if (!SetupStereoRenderTargets())
		return false;

	CreateStencil();

	SetupDistortion();

	SetupCompanionWindow();

	glGenQueries(1, &m_timeQuery);

	GLErrorCheck(__func__, __LINE__);

	return true;
}

void MainEngineGL::WaitForResolution()
{
	int spinnerInt = 0;
	char spinner[] = "-\\|/";

	while (ResolutionSettings::Get().IsSet() == false && !m_quit)
	{
		printf("Waiting for client to tell resolutions... %c\r", spinner[spinnerInt]);
		std::this_thread::sleep_for(std::chrono::seconds(1));

		if (m_server->IsConnected())
		{
			if (m_server->CheckForDisconnect())
			{
				m_quit = true;
				break;
			}
		}

		spinnerInt = (spinnerInt + 1) % sizeof(spinner);
	}

	m_width = ResolutionSettings::Get().Resolution(0).x;
	m_height = ResolutionSettings::Get().Resolution(0).y;

	m_nCompanionWindowWidth = m_width;
	m_nCompanionWindowHeight = m_height;
}

void MainEngineGL::Shutdown()
{
	free(m_frameSavingData);

	if (m_pContext)
	{
		if (m_bDebugOpenGL)
		{
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_FALSE);
			glDebugMessageCallback(nullptr, nullptr);
		}

		glDeleteBuffers(1, &m_glSceneVertBuffer);
		glDeleteBuffers(1, &m_glIDVertBuffer);
		glDeleteBuffers(1, &m_glIDIndexBuffer);

		DeleteAllShaders();

		for (int i = 0; i < FRAME_BUFFERS_ENCODING; i++)
		{
			glDeleteRenderbuffers(1, &m_leftEye[i].m_nDepthBufferId);
			glDeleteTextures(1, &m_leftEye[i].m_nRenderTextureId);
			glDeleteFramebuffers(1, &m_leftEye[i].m_nRenderFramebufferId);
			glDeleteTextures(1, &m_leftEye[i].m_nResolveTextureId);
			glDeleteFramebuffers(1, &m_leftEye[i].m_nResolveFramebufferId);

			//cudaGraphicsUnregisterResource(leftEyeDesc[i].m_cudaGraphicsResource);

			glDeleteRenderbuffers(1, &m_rightEye[i].m_nDepthBufferId);
			glDeleteTextures(1, &m_rightEye[i].m_nRenderTextureId);
			glDeleteFramebuffers(1, &m_rightEye[i].m_nRenderFramebufferId);
			glDeleteTextures(1, &m_rightEye[i].m_nResolveTextureId);
			glDeleteFramebuffers(1, &m_rightEye[i].m_nResolveFramebufferId);

			//cudaGraphicsUnregisterResource(m_rightEye[i].m_cudaGraphicsResource);
		}

		for (int j = 0; j < FRAME_BUFFERS_ENCODING; j++)
		{
			for (int i = 0; i < 2; i++)
			{
				cudaGraphicsUnregisterResource(m_cudaGraphicsResourceEye[j][i]);
			}
		}

		for (int i = 0; i < 2; i++)
		{
			if (m_encoder[i] != nullptr)
			{
				delete m_encoder[i];
			}
		}

		for (int j = 0; j < FRAME_BUFFERS_ENCODING; j++)
		{
			glDeleteBuffers(2, m_glEncoderBufferEye[j]);
		}

		if (m_unCompanionWindowVAO != 0)
		{
			glDeleteVertexArrays(1, &m_unCompanionWindowVAO);
		}
		if (m_unSceneVAO != 0)
		{
			glDeleteVertexArrays(1, &m_unSceneVAO);
		}
 		if (m_stencilBuffer != 0)
 		{
 			glDeleteVertexArrays(2, m_stencilBuffer);
 		}
 		glDeleteBuffers(2, m_stencilVAO);
		//glDeleteBuffers(1, &m_persistentBuffer);

	}

	if (m_pCompanionWindow)
	{
		SDL_DestroyWindow(m_pCompanionWindow);
		m_pCompanionWindow = NULL;
	}

	glDeleteQueries(1, &m_timeQuery);

	delete m_server;
	delete m_VRInputs;

	SDL_Quit();

#ifdef MEASURE_CUDA_MEMCPY
	m_fileOutput.close();
#endif
	//CudaErr(cuvidCtxLockDestroy(m_cudaLock));
}

void MainEngineGL::DeleteAllShaders()
{
	for (size_t i = 0; i < m_shaders.size(); i++)
	{
		delete m_shaders[i];
	}

	m_shaders.clear();
}


//  Checks client inputs, there are two each frame which is a hack

bool MainEngineGL::HandleInput(float _dt)
{
	bool bRet = false;

	static glm::vec3 debugMove = glm::vec3(0, 0.5f, 0.0f); //for text

	int clientKey = m_server->GetClientKeyPress();
	if (clientKey != 0)
		CheckInput(clientKey, bRet, debugMove);

	clientKey = m_server->GetExtraClientKeyPress();
	if (clientKey != 0)
		CheckInput(clientKey, bRet, debugMove);

	//glm::mat4 model = glm::mat4(1);
	//model = model.translate(debugMove);
	//model[5] = -1;
	////model[10] = -1;
	//m_VRInputs->SetHMDPose(model);



	return bRet;
}

void MainEngineGL::CheckInput(int keyCode, bool& bRet, glm::vec3& debugMove)
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
		m_textRenderer.DeleteShaders();
		m_textRenderer.CreateShaders();
		m_compressorMap.LoadShader();
	}
	if (keyCode == SDLK_t)
	{
		//TODO RE-read config
		//m_server->ReadConfig("../content/netConfig_Server.txt");
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
		printf("Client now sending with one frame delay\n");
	}
	if (keyCode == SDLK_p)
	{
		PrintFrame();
	}

	if (keyCode == SDLK_1)
	{
		SetResLevel(0);
	}
	if (keyCode == SDLK_2)
	{
		SetResLevel(1);
	}
	if (keyCode == SDLK_3)
	{
		SetResLevel(2);
	}
	if (keyCode == SDLK_4)
	{
		SetResLevel(3);
	}
	if (keyCode == SDLK_5)
	{
		SetResLevel(4);
	}
	if (keyCode == SDLK_6)
	{
		m_server->DisconnectClient();
		m_quit = true;
	}
	if (keyCode == SDLK_PLUS)
	{
		SetBitrate(m_encoder[0]->GetBitrate() + 1000000);
	}
	if (keyCode == SDLK_MINUS)
	{
		SetBitrate(m_encoder[0]->GetBitrate() - 1000000);
	}
}

void MainEngineGL::PrintFrame()
{
	if (USE_COMPRESSOR_MAP)
	{
		//SaveThisFrame(m_compressorMap.GetEyeTexture(0), "L");
		SaveThisFrame(m_leftEye[m_renderBufferIndex].m_nResolveTextureId, "L", m_renderBufferIndex);
	}
	else
	{
		SaveThisFrame(m_leftEye[m_resLevel_Server].m_nResolveTextureId, "L", m_resLevel_Server);
	}
	
	//SaveThisFrame(m_rightEye[m_resLevel_Server].m_nResolveTextureId, "R");
}

void MainEngineGL::SetBitrate(int _rate)
{
	if (_rate <= 0)
		return;

	//Bitrate test
	printf("Bitrate is now %d\n", _rate);

	for (int i = 0; i < 2; i++)
	{
		m_encoder[i]->ChangeBitrate(_rate);
	}
}

void MainEngineGL::Update(float _dt)
{
	//if(StatCalc::GetParameter("RES_TEST") >= 1)
	//	ResTest();

	static bool isRecording = false;

	if (!isRecording)
	{
		if (ShouldRecord())
		{
			isRecording = true;
			//SetResLevel(0);
		}
	}

	//AlarmTest();
	//BitrateTest();
	//ForgetTest();
	if (m_specialTestButton)
		LossTest();

	if (m_server->CheckForDisconnect())
		m_quit = true;
}

void MainEngineGL::AlarmTest()
{

	if (ShouldRecord())
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
	}

	//printf("Alarm %f\n", alarm);
}

void MainEngineGL::LossTest()
{
	static int counter = 0;
	static int loss = 10000;

	counter++;

	if (counter > 1000)
	{
		loss = int(loss * 0.75f);
		counter = 0;
	}

	m_server->SetSimulationLoss(loss);

	if (loss < 10)
	{
		m_server->DisconnectClient();
		m_quit = true;
	}

	//printf("Alarm %f\n", alarm);
}


//  Is the game loop

void MainEngineGL::GameLoop()
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

		//Block here for new input to render with
		WaitClientInput();
		bool inputQuit = HandleInput(fdt);

		if (inputQuit)
			m_quit = true;
		//Basically only checks for disconnects. But generic logic can be put here.
		Update(fdt);

		//Rendering
		Draw(fdt);
	}
}




//  Server draws to both eyes here and sends to encoder. 
// 

void MainEngineGL::Draw(float _dt)
{
	long long t_begin_render = 0;

	t_begin_render = Timers::Get().Time();
	long long timeStamp = m_VRInputs->GetClockTime();

	//Set frame index string
	C_RenderString* c_renderString = m_textRenderer.GetString(m_frameCounterString);
	c_renderString->Text_ptr = std::to_string(m_encoder[0]->GetCurrentFrameNr() + 1);
	c_renderString->Color = glm::vec3(1, 1, 1);

	// Render 3D scene
	RenderStereoTargets(_dt);

	if (m_server->ClientIsConnected())
	{
		//Move OpenGL textures to CUDA for encoding
		CopyToCuda(t_begin_render, timeStamp);
		printf("FPS %.2f Frame: %u %u	\r", 1.0f / _dt, m_encoder[0]->GetCurrentFrameNr(), m_encoder[1]->GetCurrentFrameNr());
	}

	if (COMPANION_WINDOW)
	{
		RenderCompanionWindow();
		SDL_GL_SwapWindow(m_pCompanionWindow);

		SDL_Event sdlEvent;
		while (SDL_PollEvent(&sdlEvent) != 0)
		{
			if (sdlEvent.type == SDL_QUIT)
			{
				m_quit = true;
			}
		}
	}
	
	glFlush();
	glFinish();

	//PrintFrame();

	GLErrorCheck(__func__, __LINE__);
}

void MainEngineGL::SaveThisFrame(GLuint _texture, const char* _id, int _frameBufferID)
{
	if (m_frameSavingData == nullptr)
	{
		m_frameSavingData = (unsigned char*)calloc(size_t(2048) * 2048 * 4,1);
	}

	memset(m_frameSavingData, 0, m_width * m_height * 4);

	//static unsigned char imgData[1200 * 1080 * 4];
	//memset(imgData, 0, 1200 * 1080 * 4);

	glFlush();
	glFinish();


	glBindTexture(GL_TEXTURE_2D, _texture);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_frameSavingData);
	glBindTexture(GL_TEXTURE_2D, 0);

	std::string name = StatCalc::GetParameterStr("SCREENSHOT_PATH");

	name += "/VR_";
	glm::ivec2 size = ResolutionSettings::Get().Resolution(_frameBufferID);
	glm::ivec2 sizePNG = size;

	name += std::to_string((int)StatCalc::GetParameter("FILE_IDENTIFIER_INT"));
	name += "_";

	name += _id;
	name += "_";

	//name += std::to_string(size.x);
	//name += "x";
	//name += std::to_string(size.y);

	//name += "_";

	//BITRATE

//	int bitrate = m_encoder[0]->GetBitrate() / 1000;

//	name += std::to_string(bitrate);
//	name += "_";

	name += std::to_string(m_encoder[0]->GetCurrentFrameNr());

	name += ".png";

	int error = PNG::WritePNG(name.c_str(), sizePNG.x, sizePNG.y, 4, m_frameSavingData);

	printf("(%d) image saved %s\n", error, name.c_str());
}

void MainEngineGL::CreateAllShaders()
{
	ShaderHandler* shader = new ShaderHandler();

	shader->CreateShaderProgram();
	shader->AddShader("../content/shaders/vsCube.vert", GL_VERTEX_SHADER);
	shader->AddShader("../content/shaders/fsCube.frag", GL_FRAGMENT_SHADER);
	shader->LinkShaderProgram();

	shader->AddUniform("PVM");
	shader->AddUniform("repeat");
	shader->AddUniform("matrixModel");
	shader->AddUniform("eyePos");
	shader->AddUniform("normalDirs");
	shader->AddUniform("isWorldMesh");

	m_shaders.push_back(shader); // 0

	shader = new ShaderHandler();

	shader->CreateShaderProgram();
	shader->AddShader("../content/shaders/vsCompanion.vert", GL_VERTEX_SHADER);
	shader->AddShader("../content/shaders/fsCompanion.frag", GL_FRAGMENT_SHADER);
	shader->LinkShaderProgram();

	m_shaders.push_back(shader); // 1

	shader = new ShaderHandler();

	shader->CreateShaderProgram();
	shader->AddShader("../content/shaders/vsDistort.vert", GL_VERTEX_SHADER);
	shader->AddShader("../content/shaders/fsDistort.frag", GL_FRAGMENT_SHADER);
	shader->LinkShaderProgram();

	//shader->AddUniform("width");
	//shader->AddUniform("height");

	m_shaders.push_back(shader); // 2

	shader = new ShaderHandler();

	shader->CreateShaderProgram();
	shader->AddShader("../content/shaders/vsStencil.vert", GL_VERTEX_SHADER);
	shader->AddShader("../content/shaders/fsStencil.frag", GL_FRAGMENT_SHADER);
	shader->LinkShaderProgram();

	m_shaders.push_back(shader); // 3
}




// 

bool MainEngineGL::SetupStereoRenderTargets()
{
	float imageSize = 1;

	//Super sampled resolution, hard coded for now
	int width = 1520;
	int height = 1680;
	//int width = m_width;
	//int height = m_height;

	printf("Resolution zero is rendered at: %dx%d\n", width, height);
	m_renderBufferIndex = ResolutionSettings::Get().AddResolution(glm::ivec2(width, height));


	//glm::ivec2 compressorRes = glm::ivec2(1232, 1680);
	//glm::ivec2 compressorRes = glm::ivec2(1520, 1360);
	if(USE_COMPRESSOR_MAP)
	{
		const glm::ivec2& compressorRes = m_compressorMap.GetCompressedSize();
		printf("Compressor map resolution: %d,%d\n", compressorRes.x, compressorRes.y);
		ResolutionSettings::Get().ModifyResolution(0,compressorRes);
		m_resLevel_Server = 0;
	}

	//Create all frame buffers
	for (int j = 0; j < FRAME_BUFFERS_ENCODING; j++)
	{
		int width = ResolutionSettings::Get().Resolution(j).x;
		int height = ResolutionSettings::Get().Resolution(j).y;
		printf("Creating Frame buffer level: %d: %dx%d\n",j, width, height);

		CreateFrameBuffer(int(width), int(height), m_leftEye[j], false);
		CreateFrameBuffer(int(width), int(height), m_rightEye[j], false);
	}

	//Create encoder, init them soon
	for (int i = 0; i < 2; i++)
	{
		m_encoder[i] = new EncoderGL();
		m_netEngine[i] = new NetworkEngine(m_server, m_encoder[i]);
	}

	//Print GPU and make cuda context
	CudaErr(cuInit(0));
	int iGpu = 0;
	CudaErr(cuDeviceGet(&m_cuDevice, iGpu));
	char szDeviceName[80];
	CudaErr(cuDeviceGetName(szDeviceName, sizeof(szDeviceName), m_cuDevice));
	std::cout << "GPU in use: " << szDeviceName << std::endl;
	for (int i = 0; i < 2; i++)
	{
		CudaErr(cuCtxCreate(&m_cuEncContext[i], CU_CTX_SCHED_AUTO, m_cuDevice));
	}

	cudaError_t eError = cudaSuccess;

	for (int j = 0; j < FRAME_BUFFERS_ENCODING; j++)
	{
		glGenBuffers(2, m_glEncoderBufferEye[j]);

		int width = ResolutionSettings::Get().Resolution(j).x;
		int height = ResolutionSettings::Get().Resolution(j).y;
		printf("Creating encoder buffer level: %d: %dx%d\n", j, width, height);

		for (int i = 0; i < 2; i++)
		{
			glBindBuffer(GL_PIXEL_PACK_BUFFER, m_glEncoderBufferEye[j][i]);
			glBufferData(GL_PIXEL_PACK_BUFFER, width * height * 4, NULL, GL_STREAM_COPY);
			glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

			CudaErr(cuCtxSetCurrent(m_cuEncContext[i]));
			eError = cudaGraphicsGLRegisterBuffer(&m_cudaGraphicsResourceEye[j][i], m_glEncoderBufferEye[j][i], CU_GRAPHICS_REGISTER_FLAGS_READ_ONLY);
			if (eError != cudaSuccess)
				break;
		}
	}

	if (eError != 0)
	{
		printf("Cuda error %d\n", eError);
		return false;
	}

	CUDA_ERROR_CHECK

	for (int i = 0; i < 2; i++)
	{
		CudaErr(cuCtxSetCurrent(m_cuEncContext[i]));
		if (m_encoder[i]->InitEncoder(m_netEngine[i], m_cuEncContext[i], i, 0) < 0)
			return false;
	}

	CudaErr(cuCtxSetCurrent(0));

	CUDA_ERROR_CHECK

	return true;
}



// 

void MainEngineGL::SetupCompanionWindow()
{
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
	unsigned int m_uiCompanionWindowIndexSize = 12;

	glGenVertexArrays(1, &m_unCompanionWindowVAO);
	glBindVertexArray(m_unCompanionWindowVAO);

	glGenBuffers(1, &m_glCompanionWindowIDVertBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_glCompanionWindowIDVertBuffer);
	glBufferData(GL_ARRAY_BUFFER, vVerts.size() * sizeof(VertexDataWindow), &vVerts[0], GL_STATIC_DRAW);

	glGenBuffers(1, &m_glCompanionWindowIDIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glCompanionWindowIDIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_uiCompanionWindowIndexSize * sizeof(GLushort), &vIndices[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataWindow), (void *)offsetof(VertexDataWindow, position));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataWindow), (void *)offsetof(VertexDataWindow, texCoord));


	//glDisableVertexAttribArray(0);
	//glDisableVertexAttribArray(1);
	glBindVertexArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}




// 

void MainEngineGL::RenderCompanionWindow()
{
	GLuint textureID = m_leftEye[m_resLevel_Server].m_nResolveTextureId;
	glm::ivec2 size = glm::ivec2(m_nCompanionWindowWidth, m_nCompanionWindowHeight);

	if (USE_COMPRESSOR_MAP)
	{
		textureID = m_compressorMap.GetEyeTexture(0);
	}

	glDisable(GL_DEPTH_TEST);
	glViewport(0, 0, size.x, size.y);

	glBindVertexArray(m_unCompanionWindowVAO);
	m_shaders[1]->UseProgram();

	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	m_textRenderer.Bind2DQuad();
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindVertexArray(0);
	glUseProgram(0);
}

// ---

//Server specific

// ---

//Called before start of every frame on the server, this blocks until a new input is receieved.
void MainEngineGL::WaitClientInput()
{
	if (m_server->ClientIsConnected())
	{
		long long lastReqTime = Timers::Get().Time();
		int requests = 0;

		while (!m_VRInputs->HasNewInput() && m_server->ClientIsConnected() && m_quit == false)
		{
			m_VRInputs->InputCondWait();

			if (m_server->CheckForDisconnect())
				m_quit = true;

		}

		if (m_server->CheckForDisconnect())
			m_quit = true;
	}
	else
	{
		std::this_thread::sleep_for(std::chrono::microseconds(16000));
	}
}


//  create a cube

void MainEngineGL::SetupScene()
{
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	std::vector<float> vertdataarray;

	AddCube(vertdataarray);

	m_uiVertcount = (unsigned int)(vertdataarray.size() / 8);

	glGenVertexArrays(1, &m_unSceneVAO);
	glBindVertexArray(m_unSceneVAO);

	glGenBuffers(1, &m_glSceneVertBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_glSceneVertBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertdataarray.size(), &vertdataarray[0], GL_STATIC_DRAW);

	GLsizei stride = sizeof(VertexDataScene);
	uintptr_t offset = 0;

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (const void *)offset);

	offset += sizeof(glm::vec3);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (const void *)offset);

	offset += sizeof(glm::vec2);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (const void *)offset);


	//glDisableVertexAttribArray(0);
	//glDisableVertexAttribArray(1);
	//glDisableVertexAttribArray(2);
	glBindVertexArray(0);
}


// 

void MainEngineGL::RenderStereoTargets(float _dt)
{
	GLErrorCheck(__func__, __LINE__);

	int renderBuffer = m_renderBufferIndex;

	if(!USE_COMPRESSOR_MAP)
	{
		if (m_resLevel_Server != 0 || NO_LENS_DISTORTION)
			renderBuffer = m_resLevel_Server;
	}

	const glm::ivec2& renderSize = ResolutionSettings::Get().Resolution(renderBuffer);

	RenderEye(m_leftEye[renderBuffer], renderSize, _dt, vr::Eye_Left);
	RenderEye(m_rightEye[renderBuffer], renderSize, _dt, vr::Eye_Right);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	if (USE_COMPRESSOR_MAP)
	{
		m_textRenderer.Bind2DQuad();
		m_compressorMap.Render(0, m_leftEye[renderBuffer].m_nResolveTextureId);
		m_compressorMap.Render(1, m_rightEye[renderBuffer].m_nResolveTextureId);
	}

	//Default FB for companion window
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	GLErrorCheck(__func__, __LINE__);

	//TEMP 20200422, might be moved to after cudacopy
	glFlush();
	glFinish();
}

void MainEngineGL::RenderEye(const FramebufferDesc& _eye, const glm::ivec2 & renderSize, float _dt, vr::EVREye _eyeId)
{
	glEnable(GL_MULTISAMPLE);

	// Left Eye
	glBindFramebuffer(GL_FRAMEBUFFER, _eye.m_nRenderFramebufferId);
	glViewport(0, 0, renderSize.x, renderSize.y);

	RenderScene(_eyeId, _dt);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDisable(GL_MULTISAMPLE);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, _eye.m_nRenderFramebufferId);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _eye.m_nResolveFramebufferId);

	glBlitFramebuffer(0, 0, renderSize.x, renderSize.y, 0, 0, renderSize.x, renderSize.y, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	if(!USE_COMPRESSOR_MAP)
	{
		if (m_resLevel_Server == 0 && !NO_LENS_DISTORTION)
			RenderDistortion(_eyeId);
	}
}

void MainEngineGL::RenderScene(vr::Hmd_Eye nEye, float _dt)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	//if (nEye == vr::EVREye::Eye_Left)
	//	RenderStencil(nEye);

	m_shaders[0]->UseProgram();
	m_shaders[0]->SetUniformV_loc("isWorldMesh", (int)StatCalc::GetParameter("USE_WORLD_MESH"));

	GLuint backgroundTexture = TEXTURE_CHECKER;

	const float roomScale = 2.2f;
	const float cubeScale = 0.1f;
	glm::vec3 center = glm::vec3(0, 3.0f * roomScale, 0);

	//glActiveTexture(GL_TEXTURE1);
	//glBindTexture(GL_TEXTURE_2D, m_compressorMap.GetCompressionTexture(nEye));
	

	RenderCube(center, nEye, backgroundTexture, glm::vec3(1.0f, 1.0f, 1.0f) * roomScale, 40.0f, -1.0f); //Inside this cube so invert normals

	RenderCube(glm::vec3(0, 0.01f, 0), nEye, TEXTURE_RED, glm::vec3(cubeScale, 0.01f, cubeScale), 5.0f);

	static glm::vec3 pos[CUBES];

	for (int i = 0; i < CUBES; i++)
	{
		static float track = 0;

		if (MOVING_SCENE)
		{
			if (FIXED_FRAME_RATE)
			{
				_dt = 1.0f / (float)StatCalc::GetParameter("FRAME_RATE");
			}

			track += (_dt) * 0.7f;
		}

		// OLD
		//if (MOVING_SCENE)
		//{
		//	static double past = omp_get_wtime();

		//	double present = omp_get_wtime();
		//	track += (present - past) * 0.7f;
		//	past = present;
		//}

		static const float RANGE = 1;
		static const float DISTANCE = 3;

		pos[i].x = (float)cos(track + i * 1.5f) * (RANGE + i);
		pos[i].y = center.y - CUBE_SIZE * roomScale + CUBE_SIZE * cubeScale;
		pos[i].z = (float)sin(track + i * 1.5f) * (RANGE + i);

		float repeat = 5.0f;

		RenderCube(pos[i], nEye, m_textures[i % m_textures.size()], glm::vec3(cubeScale, cubeScale, cubeScale), repeat);
	}

	//m_textRenderer.RenderText(m_nCompanionWindowWidth, m_nCompanionWindowHeight, 1.0f / 60.0f, 1, 1, m_VRInputs->GetPV(nEye).get(), true);
	//m_textRenderer.RenderText(m_nCompanionWindowWidth, m_nCompanionWindowHeight, 1.0f / 60.0f, 1, 1, m_VRInputs->GetPV(nEye).get(), false);

	glUseProgram(0);
}

void MainEngineGL::RenderCube(glm::vec3 &currentPos, const vr::Hmd_Eye &nEye, const GLuint &texture, const glm::vec3& _scale, float _repeat, float _normals)
{
	glm::mat4 pvm = glm::mat4(1); 
	pvm = glm::translate(pvm, currentPos);
	pvm = glm::scale(pvm, glm::vec3(_scale.x, _scale.y, _scale.z));

	glm::mat4 m = pvm;
	pvm = m_VRInputs->GetPV(nEye) * pvm;

	glm::mat4 HMDworld = glm::mat4(1); 
	m_VRInputs->GetHMDWorld(HMDworld);

	glm::vec3 eyepos = glm::vec3(HMDworld[3][0], HMDworld[3][1], HMDworld[3][2]);

	m_shaders[0]->SetUniformV_loc("normalDirs", _normals);
	m_shaders[0]->SetUniformV_loc("eyePos", eyepos);
	m_shaders[0]->SetUniformMatrix_loc("matrixModel", 1, &m[0][0]);
	m_shaders[0]->SetUniformMatrix_loc("PVM", 1, &pvm[0][0]);
	m_shaders[0]->SetUniformV_loc("repeat", _repeat);

	glBindVertexArray(m_unSceneVAO);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	glDrawArrays(GL_TRIANGLES, 0, m_uiVertcount);
	glBindVertexArray(0);
}


//Moves data from OpenGL to CUDA for the encoder on the server
void MainEngineGL::CopyToCuda(long long _t_begin_render, long long _t_input)
{
	//cudaDeviceSynchronize();
#ifdef MEASURE_CUDA_MEMCPY
	unsigned long long t0 = Timers::Get().Time();
	glBeginQuery(GL_TIME_ELAPSED, m_timeQuery);
#endif
	if (!USE_COMPRESSOR_MAP)
	{
		glBindBuffer(GL_PIXEL_PACK_BUFFER, m_glEncoderBufferEye[m_resLevel_Server][0]);
		glBindTexture(GL_TEXTURE_2D, m_leftEye[m_resLevel_Server].m_nResolveTextureId);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

		glBindBuffer(GL_PIXEL_PACK_BUFFER, m_glEncoderBufferEye[m_resLevel_Server][1]);
		glBindTexture(GL_TEXTURE_2D, m_rightEye[m_resLevel_Server].m_nResolveTextureId);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	}
	else
	{
		glBindBuffer(GL_PIXEL_PACK_BUFFER, m_glEncoderBufferEye[0][0]);
		glBindTexture(GL_TEXTURE_2D, m_compressorMap.GetEyeTexture(0));
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

		glBindBuffer(GL_PIXEL_PACK_BUFFER, m_glEncoderBufferEye[0][1]);
		glBindTexture(GL_TEXTURE_2D, m_compressorMap.GetEyeTexture(1));
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	}

	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

	//glEndQuery(GL_TIME_ELAPSED);
	//glGetQueryObjectui64v(m_timeQuery, GL_QUERY_RESULT, &m_cudaMemCpyTime);
	//m_fileOutput << m_resLevel_Server << "	" << m_cudaMemCpyTime << "\n";

	//Sync can be done here if we don't want to flush after rendering, but then we won't measure rendering time consumed.
	glFlush();
	glFinish();

	long long t_begin_encode = Timers::Get().Time();

	//GLsync sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	//glWaitSync(sync, 0, GL_TIMEOUT_IGNORED);

	size_t copySize = m_leftEye[m_resLevel_Server].m_size.x * m_leftEye[m_resLevel_Server].m_size.y * 4ULL;

	for (int i = 0; i < 2; i++)
	{
		m_encoder[i]->CopyGLtoCuda(m_cudaGraphicsResourceEye[m_resLevel_Server][i], copySize, _t_begin_render, t_begin_encode, _t_input);
	}

	//Copy measurement
#ifdef MEASURE_CUDA_MEMCPY
	glEndQuery(GL_TIME_ELAPSED);
	glGetQueryObjectui64v(m_timeQuery, GL_QUERY_RESULT, &m_cudaMemCpyTime);
	m_fileOutput << m_resLevel_Server << "	" << m_cudaMemCpyTime << "\n";

	//cudaDeviceSynchronize();
	//unsigned long long memcpyTime = Timers::Get().Time() - t0;
	//m_fileOutput << m_resLevel_Server << "	" << memcpyTime << "\n";

	

	for (int i = 0; i < 2; i++)
	{
		m_encoder[i]->GoProcess();
	}

#endif

	GLErrorCheck(__func__, __LINE__);
}

void MainEngineGL::RenderDistortion(vr::Hmd_Eye nEye)
{
	const glm::ivec2 res = ResolutionSettings::Get().Resolution(m_resLevel_Server);

	//TEMP:
	//if (res.x != 1080)
	//	return;

	glDisable(GL_DEPTH_TEST);
	glViewport(0, 0, res.x, res.y);

	glBindVertexArray(m_unLensVAO);
	m_shaders[2]->UseProgram();

	if (nEye == vr::Hmd_Eye::Eye_Left)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_leftEye[m_resLevel_Server].m_nResolveFramebufferId); //TO
		//render left lens (first half of index array )
		glBindTexture(GL_TEXTURE_2D, m_leftEye[m_renderBufferIndex].m_nResolveTextureId); //FROM
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glDrawElements(GL_TRIANGLES, GLsizei(m_uiIndexSize / 2), GL_UNSIGNED_SHORT, 0);
	}
	else
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_rightEye[m_resLevel_Server].m_nResolveFramebufferId); //TO
		//render right lens (second half of index array )
		glBindTexture(GL_TEXTURE_2D, m_rightEye[m_renderBufferIndex].m_nResolveTextureId); //FROM
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glDrawElements(GL_TRIANGLES, GLsizei(m_uiIndexSize / 2), GL_UNSIGNED_SHORT, (const void *)(m_uiIndexSize));
	}

	glBindVertexArray(0);
	glUseProgram(0);
}



void MainEngineGL::SetupDistortion()
{
	while (m_VRInputs->IsLensDataOK() == false && !m_quit)
	{
		printf("Waiting for client to tell lens distortion...\n");
		std::this_thread::sleep_for(std::chrono::seconds(1));

		if (!m_server->ClientIsConnected())
		{
			m_quit = true;
			break;
		}
	}

	GLushort m_iLensGridSegmentCountH = 43;
	GLushort m_iLensGridSegmentCountV = 43;

	std::vector<GLushort> vIndices;
	GLushort a, b, c, d;

	GLushort offset = 0;
	for (GLushort y = 0; y < m_iLensGridSegmentCountV - 1; y++)
	{
		for (GLushort x = 0; x < m_iLensGridSegmentCountH - 1; x++)
		{
			a = m_iLensGridSegmentCountH * y + x + offset;
			b = m_iLensGridSegmentCountH * y + x + 1 + offset;
			c = (y + 1)*m_iLensGridSegmentCountH + x + 1 + offset;
			d = (y + 1)*m_iLensGridSegmentCountH + x + offset;
			vIndices.push_back(a);
			vIndices.push_back(b);
			vIndices.push_back(c);

			vIndices.push_back(a);
			vIndices.push_back(c);
			vIndices.push_back(d);
		}
	}

	offset = (m_iLensGridSegmentCountH)*(m_iLensGridSegmentCountV);
	for (GLushort y = 0; y < m_iLensGridSegmentCountV - 1; y++)
	{
		for (GLushort x = 0; x < m_iLensGridSegmentCountH - 1; x++)
		{
			a = m_iLensGridSegmentCountH * y + x + offset;
			b = m_iLensGridSegmentCountH * y + x + 1 + offset;
			c = (y + 1)*m_iLensGridSegmentCountH + x + 1 + offset;
			d = (y + 1)*m_iLensGridSegmentCountH + x + offset;
			vIndices.push_back(a);
			vIndices.push_back(b);
			vIndices.push_back(c);

			vIndices.push_back(a);
			vIndices.push_back(c);
			vIndices.push_back(d);
		}
	}

	m_uiIndexSize = (GLsizei)vIndices.size();

	glGenVertexArrays(1, &m_unLensVAO);
	glBindVertexArray(m_unLensVAO);

	glGenBuffers(1, &m_glIDVertBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_glIDVertBuffer);
	glBufferData(GL_ARRAY_BUFFER, m_VRInputs->GetLensVector().size() * sizeof(VertexDataLens), &m_VRInputs->GetLensVector()[0], GL_STATIC_DRAW);

	glGenBuffers(1, &m_glIDIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIDIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, vIndices.size() * sizeof(GLushort), &vIndices[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataLens), (void *)offsetof(VertexDataLens, position));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataLens), (void *)offsetof(VertexDataLens, texCoordRed));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataLens), (void *)offsetof(VertexDataLens, texCoordGreen));

	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataLens), (void *)offsetof(VertexDataLens, texCoordBlue));


	//glDisableVertexAttribArray(0);
	//glDisableVertexAttribArray(1);
	//glDisableVertexAttribArray(2);
	//glDisableVertexAttribArray(3);
	glBindVertexArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void MainEngineGL::SetResLevel(int _level)
{
	m_resLevel_Server = _level;

	for (int i = 0; i < 2; i++)
	{
		m_encoder[i]->ChangeResolution(m_resLevel_Server);
	}
}

void MainEngineGL::ForgetTest()
{
	static double forget = 1.0;
	forget -= 0.0001;

	if (forget < 0.01)
		m_quit = true;

	//StatCalc::SetForgetFactor(forget);

	printf("forget %f\n", forget);
}

void MainEngineGL::BothFactorsTest()
{
	static double forget = 1.0;
	static double alarm = 2.0;
	static int sampleNumber = 0;

	if (forget > 0.2)
	{
		alarm -= 0.001f;

		if (sampleNumber > 1000)
		{
			forget -= 0.1;

			if (forget < 0.01)
				forget = 0.01;

			StatCalc::SetParameter("InputPktACKDelay_forget", forget);

			sampleNumber = 0;
			alarm = 2.0;
		}

		StatCalc::SetParameter("InputPktACKDelay_alarm", alarm);

		//printf("forget %f alarm %f\n", forget, alarm);

		sampleNumber++;
	}
	else
		m_quit = true;
}

bool MainEngineGL::ShouldRecord()
{
	return m_netEngine[0]->IsConnectionComplete() && m_netEngine[1]->IsConnectionComplete() && m_server->ClientIsConnected() && m_encoder[0]->GetCurrentFrameNr() > 1500;
}

void MainEngineGL::CreateStencil()
{
	const HiddenAreaMesh& stencilMeshL = m_VRInputs->GetHiddenAreaMesh(0);
	const HiddenAreaMesh& stencilMeshR = m_VRInputs->GetHiddenAreaMesh(1);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glGenVertexArrays(2, m_stencilVAO);
	glGenBuffers(2, m_stencilBuffer);

	glBindVertexArray(m_stencilVAO[0]);
	glBindBuffer(GL_ARRAY_BUFFER, m_stencilBuffer[0]);
	glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(float) * stencilMeshL.TriangleCount * 3, stencilMeshL.VertexData, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);

	glBindVertexArray(m_stencilVAO[1]);
	glBindBuffer(GL_ARRAY_BUFFER, m_stencilBuffer[1]);
	glBufferData(GL_ARRAY_BUFFER, 2* sizeof(float) * stencilMeshR.TriangleCount * 3, stencilMeshR.VertexData, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//Render all stencils once
	for (int i = 0; i < FRAME_BUFFERS_ENCODING; i++)
	{
		const glm::ivec2& renderSize = ResolutionSettings::Get().Resolution(i);
		glViewport(0, 0, renderSize.x, renderSize.y);

		glBindFramebuffer(GL_FRAMEBUFFER, m_leftEye[i].m_nRenderFramebufferId);
		RenderStencil(vr::Eye_Left);

		glBindFramebuffer(GL_FRAMEBUFFER, m_rightEye[i].m_nRenderFramebufferId);
		RenderStencil(vr::Eye_Right);
	}

	GLErrorCheck(__func__, __LINE__);
}

void MainEngineGL::RenderStencil(vr::EVREye _eye)
{
	//glClearColor(0, 0, 0, 1);
	//glClearStencil(0);

	m_shaders[3]->UseProgram();

	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // Do not draw any pixels on the back buffer
	glEnable(GL_STENCIL_TEST); // Enables testing AND writing functionalities
	glStencilFunc(GL_EQUAL, 1, 0xFF); // Do not test the current value in the stencil buffer, always accept any value on there for drawing
	glStencilMask(0xFF);
	glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE); // Make every test succeed

	//Both eyes are the same tri count
	const HiddenAreaMesh& stencilMesh = m_VRInputs->GetHiddenAreaMesh(_eye); 

	glBindVertexArray(m_stencilVAO[_eye]);
	glDrawArrays(GL_TRIANGLES, 0, stencilMesh.TriangleCount * 3);

	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP); // Make sure you will no longer (over)write stencil values, even if any test succeeds
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); // Make sure we draw on the backbuffer again.
	glStencilFunc(GL_EQUAL, 0, 0xFF); // Now we will only draw pixels where the corresponding stencil buffer value equals 1

	glBindVertexArray(0);

	GLErrorCheck(__func__, __LINE__);
}