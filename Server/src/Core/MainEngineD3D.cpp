#include "MainEngineD3D.h"
#include <PNG.h>
#include <driver_types.h>
#include <cuda_d3d11_interop.h>
#include <CrashDump.h>

MainEngineD3D::MainEngineD3D(int argc, char *argv[])
{
}

MainEngineD3D::~MainEngineD3D()
{
}

bool MainEngineD3D::Init()
{
	printf("USING D3D11\n\n");
	m_VRInputs = new VRInputs();
	m_server = new Server(m_VRInputs);

	MOVING_SCENE = StatCalc::GetParameter("MOVING_SCENE") > 0;
	NO_LENS_DISTORTION = StatCalc::GetParameter("NO_LENS_DISTORTION") > 0;
	COMPANION_WINDOW = StatCalc::GetParameter("COMPANION_WINDOW") > 0;
	FIXED_FRAME_RATE = StatCalc::GetParameter("FIXED_FRAME_RATE") > 0;
	USE_COMPRESSOR_MAP = StatCalc::GetParameter("USE_COMPRESSOR_MAP") > 0;
	USE_D3D11 = StatCalc::GetParameter("USE_D3D11") > 0;
	USE_WORLD_MESH = StatCalc::GetParameter("USE_WORLD_MESH") > 0;

	if (m_server->InitNetwork() < 0)
		return false;

	WaitForResolution();

	SetProcessDPIAware();

#ifdef LOCAL_TESTING_ONLY
	m_renderer.InitD3D(m_width, m_height);
	m_renderer.SetVRInput(m_VRInputs);
#else
	m_renderer.InitD3D(m_width, m_height);
	m_renderer.SetVRInput(m_VRInputs);

	//m_renderer[1].InitD3D(m_width, m_height, m_renderer.GetDevice(), m_renderer.GetDeviceContext());
	//m_renderer[1].SetVRInput(m_VRInputs);
#endif

	InitGL();

	return true;
}


//  Initialize OpenGL. Returns true if OpenGL has been successfully
//          initialized, false if shaders could not be created.
//          If failure occurred in a module other than shaders, the function
//          may return true or throw an error. 

bool MainEngineD3D::InitGL()
{
	if (USE_COMPRESSOR_MAP)
	{
		//for (int i = 0; i < RENDERERS; i++)
		{
			m_compressorMap.LoadMaps(m_renderer.GetDevice(), m_renderer.GetDeferredDeviceContext());
		}
	}


	SetupScene();

	m_textures["../content/textures/red.png"] = nullptr;
	m_textures["../content/textures/blue.png"] = nullptr;
	m_textures["../content/textures/gray.png"] = nullptr;
	m_textures["../content/textures/yellow.png"] = nullptr;
	m_textures["../content/textures/checker.png"] = nullptr;

	if (USE_WORLD_MESH)
	{
		m_textures["../content/textures/box.png"] = nullptr;
	}

	for (auto it = m_textures.begin(); it != m_textures.end(); it++)
	{
		D3D11_TEXTURE_ADDRESS_MODE mode = D3D11_TEXTURE_ADDRESS_WRAP;

		if (USE_WORLD_MESH)
		{
			mode = D3D11_TEXTURE_ADDRESS_CLAMP;
		}

		m_renderer.LoadTexture(it->first.c_str(), &it->second, mode);
	}

	if (!SetupStereoRenderTargets())
		return false;

	CreateStencil();

	SetupDistortion();


	//glGenQueries(1, &m_timeQuery);


	return true;
}



//  create a cube

void MainEngineD3D::SetupScene()
{
	std::vector<float> vertdataarray;
	
	if (USE_WORLD_MESH)
	{
		AddCubeMap(vertdataarray);
		//int id = AssetLoader::LoadModel("../content/world.mff");
		//int vertices = (AssetLoader::GetModel(id).VertexDataCount / 8);
		//m_renderer.CreateVertexBuffer(AssetLoader::GetModel(id).VertexData, m_worldModel, 8, AssetLoader::GetModel(id).VertexDataCount / 8);
	}
	else
	{
		AddCube(vertdataarray);
	}

	m_uiVertcount = (unsigned int)(vertdataarray.size() / 8);
	m_renderer.CreateVertexBuffer(vertdataarray.data(), m_cubeModel, 8, (UINT)vertdataarray.size() / 8);

	

}

void MainEngineD3D::WaitForResolution()
{
	int spinnerInt = 0;
	char spinner = '/';

	while (ResolutionSettings::Get().IsSet() == false && !m_quit)
	{
		printf("Waiting for client to tell resolutions... %c\r", spinner);
		std::this_thread::sleep_for(std::chrono::seconds(1));

		if (m_server->IsConnected())
		{
			if (m_server->CheckForDisconnect())
			{
				m_quit = true;
				break;
			}
		}

		if (spinnerInt == 0)
			spinner = '-';
		else if (spinnerInt == 1)
			spinner = '\\';
		else if (spinnerInt == 2)
			spinner = '|';
		else if (spinnerInt == 3)
			spinner = '/';
		else if (spinnerInt == 4)
			spinner = '-';
		else if (spinnerInt == 5)
			spinner = '\\';
		else if (spinnerInt == 6)
			spinner = '|';
		else if (spinnerInt == 7)
		{
			spinner = '/';
			spinnerInt = -1;
		}
		spinnerInt++;
	}

	m_width = ResolutionSettings::Get().Resolution(0).x;
	m_height = ResolutionSettings::Get().Resolution(0).y;

	m_nCompanionWindowWidth = m_width;
	m_nCompanionWindowHeight = m_height;
}

void MainEngineD3D::Shutdown()
{
	cudaGraphicsUnregisterResource(m_cudaResource[0]);
	cudaGraphicsUnregisterResource(m_cudaResource[1]);

	free(m_frameSavingData);

	if (m_printingTexture != nullptr)
		m_printingTexture->Release();

#ifdef MEASURE_CUDA_MEMCPY
	m_fileOutput.close();
#endif

	m_netEngine[0]->DestroyWorker();
	m_netEngine[1]->DestroyWorker();
	m_encoder[0]->DestroyWorker();
	m_encoder[1]->DestroyWorker();
	m_server->Destroy();

	delete m_server;
	delete m_VRInputs;

	delete m_encoder[0];
	delete m_encoder[1];

	printf("Shutdown");

}


//  Checks client inputs, there are two each frame which is a hack

bool MainEngineD3D::HandleInput(float _dt)
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

void MainEngineD3D::CheckInput(int keyCode, bool& bRet, glm::vec3& debugMove)
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
		m_renderer.CreateShaders();

		if (USE_COMPRESSOR_MAP)
		{
			m_compressorMap.LoadShader();
		}
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

void MainEngineD3D::PrintFrame()
{
	SaveThisFrame("L");
}

void MainEngineD3D::SetBitrate(int _rate)
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

void MainEngineD3D::Update(float _dt)
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

void MainEngineD3D::AlarmTest()
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

void MainEngineD3D::LossTest()
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

void MainEngineD3D::GameLoop()
{
	__try
	//try
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
#ifndef LOCAL_TESTING_ONLY
			WaitClientInput();
#endif
			bool inputQuit = HandleInput(fdt);

			if (inputQuit)
				m_quit = true;
			//Basically only checks for disconnects. But generic logic can be put here.
			Update(fdt);

			//Rendering
			Draw(fdt);
		}
	}
	//catch (...)
	__except (Crash(GetExceptionInformation()), EXCEPTION_EXECUTE_HANDLER)
	{
		//Crash(GetExceptionInformation());
	}
}




//  Server draws to both eyes here and sends to encoder. 
// 

void MainEngineD3D::Draw(float _dt)
{
	long long t_begin_render = 0;
	long long t_begin_encode = 0;

	t_begin_render = Timers::Get().Time();
	long long timeStamp = m_VRInputs->GetClockTime();

	//m_renderer.TimerStart();

	//Set frame index string
	//C_RenderString* c_renderString = m_textRenderer.GetString(m_frameCounterString);
	//c_renderString->Text_ptr = std::to_string(m_encoder[0]->GetCurrentFrameNr() + 1);
	//c_renderString->Color = glm::vec3(1, 1, 1);

	// Render 3D scene
	RenderStereoTargets(_dt);

	//m_renderer.TimerEnd(); //blocks

	t_begin_encode = Timers::Get().Time();

	if (m_server->ClientIsConnected())
	{
		//Move OpenGL textures to CUDA for encoding
#ifndef LOCAL_TESTING_ONLY
		CopyToCuda(t_begin_render, t_begin_encode, timeStamp);
#endif
		printf("FPS %.2f Frame: %u %u	\r", 1.0f / _dt, m_encoder[0]->GetCurrentFrameNr(), m_encoder[1]->GetCurrentFrameNr());
	}

	//if (COMPANION_WINDOW)
	//{
	//	m_renderer.DrawCompanionWindow();
	//	//SDL_GL_SwapWindow(m_pCompanionWindow);

	//	SDL_Event sdlEvent;
	//	while (SDL_PollEvent(&sdlEvent) != 0)
	//	{
	//		if (sdlEvent.type == SDL_QUIT)
	//		{
	//			m_quit = true;
	//		}
	//	}
	//}
	
	//glFlush();
	//glFinish();

	//PrintFrame();

	//GLErrorCheck(__func__, __LINE__);
}

void MainEngineD3D::SaveThisFrame(const char* _id)
{
	if (m_frameSavingData == nullptr)
	{
		m_frameSavingData = (unsigned char*)calloc(size_t(2048) * 2048 * 4,1);
	}

	//int width = ResolutionSettings::Get().Resolution(m_compressorBufferIndex).x;
	//int height = ResolutionSettings::Get().Resolution(m_compressorBufferIndex).y;
	//m_renderer.CopyTexture(m_printingTexture, m_compressorMap.GetEncodedTexture(0), m_frameSavingData, width, height, 4);

	int width = m_eye[m_renderBufferIndex][0].Width;
	int height = m_eye[m_renderBufferIndex][0].Height;
	m_renderer.CopyTexture(m_printingTexture, m_eye[m_renderBufferIndex][0].FrameTexture, m_frameSavingData, width, height, 4);

	std::string name = StatCalc::GetParameterStr("SCREENSHOT_PATH");

	name += "/VR_";

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

	int error = PNG::WritePNG(name.c_str(), width, height, 4, m_frameSavingData);

	printf("(%d) image saved %s\n", error, name.c_str());
}


// 

bool MainEngineD3D::SetupStereoRenderTargets()
{
	float imageSize = 1;

	//glm::ivec2 compressorRes = glm::ivec2(1232, 1680);
	//glm::ivec2 compressorRes = glm::ivec2(1520, 1360);

	//Super sampled resolution, hard coded for now
	int width = 1520;
	int height = 1680;
	printf("Resolution zero is rendered at: %dx%d\n", width, height);
	m_renderBufferIndex = ResolutionSettings::Get().AddResolution(glm::ivec2(width, height));

	if (USE_COMPRESSOR_MAP)
	{
		const glm::ivec2& compressorRes = m_compressorMap.GetCompressedSize();
		printf("Compressor map resolution: %d,%d\n", compressorRes.x, compressorRes.y);
		m_compressorBufferIndex = ResolutionSettings::Get().AddResolution(compressorRes);
		m_resLevel_Server = m_compressorBufferIndex;
	}

	//Create all frame buffers
	for (int j = 0; j < FRAME_BUFFERS_ENCODING; j++)
	{
		width = ResolutionSettings::Get().Resolution(j).x;
		height = ResolutionSettings::Get().Resolution(j).y;
		printf("Creating Frame buffer level: %d: %dx%d\n", j, width, height);

		for (int i = 0; i < 2; i++)
		{
			m_renderer.CreateFrameBuffer(m_eye[j][i], width, height);
		}
	}

	{
		//Create print texture
		D3D11_TEXTURE2D_DESC texDesc;

		texDesc.Width = ResolutionSettings::Get().Resolution(m_renderBufferIndex).x;
		texDesc.Height = ResolutionSettings::Get().Resolution(m_renderBufferIndex).y;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		texDesc.SampleDesc.Count = 1;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage = D3D11_USAGE_STAGING;
		texDesc.BindFlags = 0;
		texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		texDesc.MiscFlags = 0;

		HRESULT res = m_renderer.CreateTexture2D(&texDesc, &m_printingTexture);

		if (res < 0)
		{
			printf("Failed to create printing texture %d\n", res);
		}

	}


	//g_texture_2d.width = 256;
	//g_texture_2d.height = 256;

	//D3D11_TEXTURE2D_DESC desc;
	//ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
	//desc.Width = g_texture_2d.width;
	//desc.Height = g_texture_2d.height;
	//desc.MipLevels = 1;
	//desc.ArraySize = 1;
	//desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	//desc.SampleDesc.Count = 1;
	//desc.Usage = D3D11_USAGE_DEFAULT;
	//desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;



	//Print GPU and make cuda context
	CudaErr(cuInit(0));
	int iGpu = 0;
	CudaErr(cuDeviceGet(&m_cuDevice, iGpu));
	char szDeviceName[80];
	CudaErr(cuDeviceGetName(szDeviceName, sizeof(szDeviceName), m_cuDevice));
	std::cout << "CUDA GPU in use: " << szDeviceName << std::endl;

	for (int i = 0; i < 2; i++)
	{
		CudaErr(cuCtxCreate(&m_cuEncContext[i], CU_CTX_SCHED_AUTO, m_cuDevice));
	}

	//cudaError_t eError = cudaSuccess;

	//for (int j = 0; j < FRAME_BUFFERS_ENCODING; j++)
	//{
	//	//glGenBuffers(2, m_glEncoderBufferEye[j]);

	//	int width = ResolutionSettings::Get().Resolution(j).x;
	//	int height = ResolutionSettings::Get().Resolution(j).y;
	//	printf("Creating encoder buffer level: %d: %dx%d\n", j, width, height);

	//for (int i = 0; i < 2; i++)
	//{
	//	//glBindBuffer(GL_PIXEL_PACK_BUFFER, m_glEncoderBufferEye[j][i]);
	//	//glBufferData(GL_PIXEL_PACK_BUFFER, width * height * 4, NULL, GL_STREAM_COPY);
	//	//glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

	//	CudaErr(cuCtxSetCurrent(m_cuEncContext[i]));

	//	if (USE_COMPRESSOR_MAP)
	//	{
	//		CudaErr(cudaGraphicsD3D11RegisterResource(&m_cudaResource[i], m_compressorMap.GetEncodedTexture(i), cudaGraphicsRegisterFlagsNone));
	//	}
	//	else
	//	{
	//		CudaErr(cudaGraphicsD3D11RegisterResource(&m_cudaResource[i], m_eye[m_resLevel_Server][0].FrameTexture, cudaGraphicsRegisterFlagsNone));
	//	}
	//	//eError = cudaGraphicsGLRegisterBuffer(&m_cudaGraphicsResourceEye[j][i], m_glEncoderBufferEye[j][i], CU_GRAPHICS_REGISTER_FLAGS_READ_ONLY);
	//}
	

	//if (eError != 0)
	//{
	//	printf("Cuda error %d\n", eError);
	//	return false;
	//}

	CUDA_ERROR_CHECK

	for (int i = 0; i < 2; i++)
	{
		
		
#ifdef D3D_ENCODER
		m_encoder[i] = new EncoderD3D();
		m_netEngine[i] = new NetworkEngine(m_server, (EncoderBase*)m_encoder[i]);
		m_encoder[i]->InitEncoder(m_netEngine[i], i, m_resLevel_Server, m_renderer.GetDevice(), m_renderer.GetDeviceContext());
#else
		m_encoder[i] = new EncLowLatency_v11();
		m_netEngine[i] = new NetworkEngine(m_server, (EncoderBase*)m_encoder[i]);
		m_encoder[i]->InitEncoder(m_netEngine[i], m_cuEncContext[i], i, m_resLevel_Server);
#endif
	}

	CudaErr(cuCtxSetCurrent(0));

	//CUDA_ERROR_CHECK

	return true;
}

//Called before start of every frame on the server, this blocks until a new input is receieved.
void MainEngineD3D::WaitClientInput()
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

void MainEngineD3D::RenderStereoTargets(float _dt)
{
	const glm::ivec2& renderSize = ResolutionSettings::Get().Resolution(m_renderBufferIndex);

	for (int i = 0; i < 2; i++)
	{
		//m_renderer.RenderDepthStencil(m_eye[renderBuffer][i], i);

		RenderEye(renderSize, _dt, vr::Hmd_Eye(i));

		if (USE_COMPRESSOR_MAP)
		{
			//m_compressorMap.Render(m_eye[m_renderBufferIndex][i], i);
			m_compressorMap.Render(m_eye[m_renderBufferIndex][i], i);
		}
	}

#ifdef D3D_ENCODER
	//m_encoder[i]->CudaLock();
	m_renderer.EndDraw();
	//m_encoder[i]->CudaUnlock();
#endif
#ifdef LOCAL_TESTING_ONLY
	m_renderer.Present();
#endif
}

void MainEngineD3D::RenderEye(const glm::ivec2 & renderSize, float _dt, vr::EVREye _eyeId)
{
	//glEnable(GL_MULTISAMPLE);

	//// Left Eye
	//glBindFramebuffer(GL_FRAMEBUFFER, _eye.m_nRenderFramebufferId);
	//glViewport(0, 0, renderSize.x, renderSize.y);

	if (USE_WORLD_MESH)
	{
		float scale = 10;
		m_renderer.DrawCubesBegin(m_eye[m_renderBufferIndex][_eyeId], m_cubeModel, m_renderer.GetDeferredDeviceContext());
		m_renderer.DrawCube(_eyeId, -1.0f, glm::vec3(0,0,0), glm::vec3(1.0f, 1.0f, 1.0f) * scale, m_textures["../content/textures/box.png"], m_renderer.GetDeferredDeviceContext(), m_cubeModel, 1.0f);
		m_renderer.SetCullFront(false, m_renderer.GetDeferredDeviceContext());
	}
	else
	{
		m_renderer.DrawCubesBegin(m_eye[m_renderBufferIndex][_eyeId], m_cubeModel, m_renderer.GetDeferredDeviceContext());
		RenderScene(_eyeId, _dt);
	}

	


	//TODO: WITH MULTI SAMPLING
	/*glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDisable(GL_MULTISAMPLE);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, _eye.m_nRenderFramebufferId);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _eye.m_nResolveFramebufferId);

	glBlitFramebuffer(0, 0, renderSize.x, renderSize.y, 0, 0, renderSize.x, renderSize.y, GL_COLOR_BUFFER_BIT, GL_LINEAR);*/

	//TODO: WITH LENS DISTORTION
	/*if(!USE_COMPRESSOR_MAP)
	{
		if (m_resLevel_Server == 0 && !NO_LENS_DISTORTION)
			RenderDistortion(_eyeId);
	}*/
}

void MainEngineD3D::RenderScene(vr::Hmd_Eye _eye, float _dt)
{
	const float roomScale = 2.2f;
	const float cubeScale = 0.1f;
	glm::vec3 center = glm::vec3(0, 3.0f * roomScale, 0);
	
	m_renderer.DrawCube(_eye, -1.0f, center, glm::vec3(1.0f, 1.0f, 1.0f) * roomScale, m_textures["../content/textures/checker.png"], m_renderer.GetDeferredDeviceContext(), m_cubeModel, 40.0f );
	m_renderer.DrawCube(_eye, 1.0f, glm::vec3(0, 0.01f, 0), glm::vec3(cubeScale, 0.01f, cubeScale), m_textures["../content/textures/red.png"], m_renderer.GetDeferredDeviceContext(), m_cubeModel, 1.0f);


	static glm::vec3 pos[CUBES];

	auto textureIterator = m_textures.begin();

	for (int i = 0; i < CUBES; i++)
	{
		if(textureIterator == m_textures.end())
			textureIterator = m_textures.begin();

		static float track = 0;

		if (MOVING_SCENE)
		{
			if (FIXED_FRAME_RATE)
			{
				_dt = 1.0f / (float)StatCalc::GetParameter("FRAME_RATE");
			}

			track += (_dt) * 0.7f;
		}

		static const float RANGE = 1;
		static const float DISTANCE = 3;

		pos[i].x = (float)cos(track + i * 1.5f) * (RANGE + i);
		pos[i].y = center.y - CUBE_SIZE * roomScale + CUBE_SIZE * cubeScale;
		pos[i].z = (float)sin(track + i * 1.5f) * (RANGE + i);

		m_renderer.DrawCube(_eye, 1.0f, pos[i], glm::vec3(cubeScale, cubeScale, cubeScale), textureIterator->second, m_renderer.GetDeferredDeviceContext(), m_cubeModel, 5.0f);
		textureIterator++;

	}
}

//Moves data from OpenGL to CUDA for the encoder on the server
void MainEngineD3D::CopyToCuda(long long _t_begin_render, long long _t_begin_encode, long long _t_input)
{
	for (int i = 0; i < 2; i++)
	{
#ifdef D3D_ENCODER
		ID3D11Texture2D* texture = m_eye[m_renderBufferIndex][i].FrameTexture;
		if (USE_COMPRESSOR_MAP)
		{
			texture = m_compressorMap.GetEncodedTexture(i);
		}
		m_encoder[i]->StartEncoding(_t_begin_render, _t_begin_encode, _t_input, texture);
#else
		size_t copySize = m_eye[m_renderBufferIndex][i].Width * m_eye[m_renderBufferIndex][i].Height * 4ULL;
		m_encoder[i]->CopyGLtoCuda(m_cudaResource[i], copySize,  _t_begin_render, _t_begin_encode, _t_input);
#endif
	}

	for (int i = 0; i < 2; i++)
	{
		m_encoder[i]->GoProcess();
	}
}
//
//void MainEngineD3D::RenderDistortion(vr::Hmd_Eye nEye)
//{
//	const glm::ivec2 res = ResolutionSettings::Get().Resolution(m_resLevel_Server);
//
//	//TEMP:
//	//if (res.x != 1080)
//	//	return;
//
//	glDisable(GL_DEPTH_TEST);
//	glViewport(0, 0, res.x, res.y);
//
//	glBindVertexArray(m_unLensVAO);
//	m_shaders[2]->UseProgram();
//
//	if (nEye == vr::Hmd_Eye::Eye_Left)
//	{
//		glBindFramebuffer(GL_FRAMEBUFFER, m_leftEye[m_resLevel_Server].m_nResolveFramebufferId); //TO
//		//render left lens (first half of index array )
//		glBindTexture(GL_TEXTURE_2D, m_leftEye[RENDER_BUFFER_INDEX].m_nResolveTextureId); //FROM
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
//		glDrawElements(GL_TRIANGLES, GLsizei(m_uiIndexSize / 2), GL_UNSIGNED_SHORT, 0);
//	}
//	else
//	{
//		glBindFramebuffer(GL_FRAMEBUFFER, m_rightEye[m_resLevel_Server].m_nResolveFramebufferId); //TO
//		//render right lens (second half of index array )
//		glBindTexture(GL_TEXTURE_2D, m_rightEye[RENDER_BUFFER_INDEX].m_nResolveTextureId); //FROM
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
//		glDrawElements(GL_TRIANGLES, GLsizei(m_uiIndexSize / 2), GL_UNSIGNED_SHORT, (const void *)(m_uiIndexSize));
//	}
//
//	glBindVertexArray(0);
//	glUseProgram(0);
//}

void MainEngineD3D::SetupDistortion()
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

	//TODO: LENS DISTORTION
	//GLushort m_iLensGridSegmentCountH = 43;
	//GLushort m_iLensGridSegmentCountV = 43;

	//std::vector<GLushort> vIndices;
	//GLushort a, b, c, d;

	//GLushort offset = 0;
	//for (GLushort y = 0; y < m_iLensGridSegmentCountV - 1; y++)
	//{
	//	for (GLushort x = 0; x < m_iLensGridSegmentCountH - 1; x++)
	//	{
	//		a = m_iLensGridSegmentCountH * y + x + offset;
	//		b = m_iLensGridSegmentCountH * y + x + 1 + offset;
	//		c = (y + 1)*m_iLensGridSegmentCountH + x + 1 + offset;
	//		d = (y + 1)*m_iLensGridSegmentCountH + x + offset;
	//		vIndices.push_back(a);
	//		vIndices.push_back(b);
	//		vIndices.push_back(c);

	//		vIndices.push_back(a);
	//		vIndices.push_back(c);
	//		vIndices.push_back(d);
	//	}
	//}

	//offset = (m_iLensGridSegmentCountH)*(m_iLensGridSegmentCountV);
	//for (GLushort y = 0; y < m_iLensGridSegmentCountV - 1; y++)
	//{
	//	for (GLushort x = 0; x < m_iLensGridSegmentCountH - 1; x++)
	//	{
	//		a = m_iLensGridSegmentCountH * y + x + offset;
	//		b = m_iLensGridSegmentCountH * y + x + 1 + offset;
	//		c = (y + 1)*m_iLensGridSegmentCountH + x + 1 + offset;
	//		d = (y + 1)*m_iLensGridSegmentCountH + x + offset;
	//		vIndices.push_back(a);
	//		vIndices.push_back(b);
	//		vIndices.push_back(c);

	//		vIndices.push_back(a);
	//		vIndices.push_back(c);
	//		vIndices.push_back(d);
	//	}
	//}

	//m_uiIndexSize = (GLsizei)vIndices.size();

	//glGenVertexArrays(1, &m_unLensVAO);
	//glBindVertexArray(m_unLensVAO);

	//glGenBuffers(1, &m_glIDVertBuffer);
	//glBindBuffer(GL_ARRAY_BUFFER, m_glIDVertBuffer);
	//glBufferData(GL_ARRAY_BUFFER, m_VRInputs->GetLensVector().size() * sizeof(VertexDataLens), &m_VRInputs->GetLensVector()[0], GL_STATIC_DRAW);

	//glGenBuffers(1, &m_glIDIndexBuffer);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIDIndexBuffer);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, vIndices.size() * sizeof(GLushort), &vIndices[0], GL_STATIC_DRAW);

	//glEnableVertexAttribArray(0);
	//glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataLens), (void *)offsetof(VertexDataLens, position));

	//glEnableVertexAttribArray(1);
	//glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataLens), (void *)offsetof(VertexDataLens, texCoordRed));

	//glEnableVertexAttribArray(2);
	//glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataLens), (void *)offsetof(VertexDataLens, texCoordGreen));

	//glEnableVertexAttribArray(3);
	//glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataLens), (void *)offsetof(VertexDataLens, texCoordBlue));

	//glBindVertexArray(0);

	//glBindBuffer(GL_ARRAY_BUFFER, 0);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void MainEngineD3D::SetResLevel(int _level)
{
	m_resLevel_Server = _level;

	for (int i = 0; i < 2; i++)
	{
		m_encoder[i]->ChangeResolution(m_resLevel_Server);
	}
}

bool MainEngineD3D::ShouldRecord()
{
	return m_netEngine[0]->IsConnectionComplete() && m_netEngine[1]->IsConnectionComplete() && m_server->ClientIsConnected() && m_encoder[0]->GetCurrentFrameNr() > 1500;
}

void MainEngineD3D::CreateStencil()
{
	const HiddenAreaMesh& stencilMeshL = m_VRInputs->GetHiddenAreaMesh(0);
	const HiddenAreaMesh& stencilMeshR = m_VRInputs->GetHiddenAreaMesh(1);

	/*glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

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
	glBindVertexArray(0);*/

	//Render all stencils once
	for (int i = 0; i < FRAME_BUFFERS_ENCODING; i++)
	{
		//const glm::ivec2& renderSize = ResolutionSettings::Get().Resolution(i);
		//glViewport(0, 0, renderSize.x, renderSize.y);

		//glBindFramebuffer(GL_FRAMEBUFFER, m_leftEye[i].m_nRenderFramebufferId);
		//RenderStencil(vr::Eye_Left);

		//glBindFramebuffer(GL_FRAMEBUFFER, m_rightEye[i].m_nRenderFramebufferId);
		//RenderStencil(vr::Eye_Right);
		for (int j = 0; j < 2; j++)
		{
			m_renderer.RenderDepthStencil(m_eye[i][j], j);
		}
	}

	//GLErrorCheck(__func__, __LINE__);
}
//
//void MainEngineD3D::RenderStencil(vr::EVREye _eye)
//{
//	//glClearColor(0, 0, 0, 1);
//	//glClearStencil(0);
//
//	m_shaders[3]->UseProgram();
//
//	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // Do not draw any pixels on the back buffer
//	glEnable(GL_STENCIL_TEST); // Enables testing AND writing functionalities
//	glStencilFunc(GL_EQUAL, 1, 0xFF); // Do not test the current value in the stencil buffer, always accept any value on there for drawing
//	glStencilMask(0xFF);
//	glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE); // Make every test succeed
//
//	//Both eyes are the same tri count
//	const HiddenAreaMesh& stencilMesh = m_VRInputs->GetHiddenAreaMesh(_eye); 
//
//	glBindVertexArray(m_stencilVAO[_eye]);
//	glDrawArrays(GL_TRIANGLES, 0, stencilMesh.TriangleCount * 3);
//
//	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP); // Make sure you will no longer (over)write stencil values, even if any test succeeds
//	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); // Make sure we draw on the backbuffer again.
//	glStencilFunc(GL_EQUAL, 0, 0xFF); // Now we will only draw pixels where the corresponding stencil buffer value equals 1
//
//	glBindVertexArray(0);
//
//	GLErrorCheck(__func__, __LINE__);
//}