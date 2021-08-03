/*
* 
*/

#include "VRInputs.h"
#include <gtc/matrix_transform.hpp>
#include "../Utility/Timers.h"

#include <SDL.h>

VRInputs::VRInputs(vr::IVRSystem * _hmd)
{
	m_pHMD = _hmd;
}

VRInputs::~VRInputs()
{
	if (m_pHMD == NULL)
		delete m_camera;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void VRInputs::SetupCameras(int _w, int _h)
{
	ROTATING_VIEW = StatCalc::GetParameter("ROTATING_VIEW") > 0;
	MOUSE_LOOK = StatCalc::GetParameter("MOUSE_LOOK") > 0;
	ROTATION_FROM_FILE_TEST = StatCalc::GetParameter("ROTATION_FROM_FILE_TEST") > 0;

	if (ROTATION_FROM_FILE_TEST)
	{
		LoadRotationTestFile();
	}

	if (m_pHMD == NULL)
	{
		m_camera = new GLCamera(110, _w, _h, 0.1f, 300.0f);
		SDL_SetRelativeMouseMode(SDL_TRUE);
	}

	m_mat4ProjectionLeft = GetHMDMatrixProjectionEye(vr::Eye_Left, _w,_h);
	m_mat4ProjectionRight = GetHMDMatrixProjectionEye(vr::Eye_Right,_w,_h);
	m_mat4eyePosLeft = GetHMDMatrixPoseEye(vr::Eye_Left);
	m_mat4eyePosRight = GetHMDMatrixPoseEye(vr::Eye_Right);
}

void VRInputs::LoadRotationTestFile()
{
	m_fileRotationIndex = 60000;

	std::ifstream rotationTestFile("../content/rotationTest.txt", std::ifstream::in);

	if (rotationTestFile.is_open())
	{
		int line = 0;

		while (!rotationTestFile.eof())
		{
			bool wasError = 0;

			std::string temp;
			std::string x;
			std::string y;
			std::string z;

			for (int i = 0; i < 5; i++)
			{
				rotationTestFile >> temp;
				if (i == 0 && temp != "h")
				{
					wasError = true;
					printf("Error in rotation file! Line %d\n", line);
					break;
				}

			}

			if (wasError)
				break;

			rotationTestFile >> x >> y >> z;

			glm::vec3 rotation;

			rotation.x = std::stof(x);
			rotation.y = std::stof(y);
			rotation.z = std::stof(z);

			m_fileRotations.push_back(rotation);

			line++;
		}
	}
	else
	{
		printf("Failed to open test rotation file!\n");
	}
}

//-----------------------------------------------------------------------------
// Purpose: Gets a Matrix Projection Eye with respect to nEye.
//-----------------------------------------------------------------------------
glm::mat4 VRInputs::GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye, int _w, int _h)
{
	if (m_pHMD != NULL)
	{
		if (!m_pHMD)
			return glm::mat4(1);

		printf("Using VR camera projection\n");

		float fNearClip = 0.1f;
		float fFarClip = 100.0f;

		vr::HmdMatrix44_t mat = m_pHMD->GetProjectionMatrix(nEye, fNearClip, fFarClip);

		return glm::mat4(
			mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
			mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
			mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
			mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]
		);
	}
	else
	{
		glm::mat4 matproj;

		printf("Using mouse camera projection\n");

		memcpy(&matproj[0], m_camera->GetProjection(), 16 * 4);
		return matproj;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Gets an HMDMatrixPoseEye with respect to nEye.
//-----------------------------------------------------------------------------
glm::mat4 VRInputs::GetHMDMatrixPoseEye(vr::Hmd_Eye nEye)
{
	if (!m_pHMD)
		return glm::mat4(1);

	vr::HmdMatrix34_t matEyeRight = m_pHMD->GetEyeToHeadTransform(nEye);

	glm::mat4 matrixObj(
		matEyeRight.m[0][0], matEyeRight.m[1][0], matEyeRight.m[2][0], 0.0,
		matEyeRight.m[0][1], matEyeRight.m[1][1], matEyeRight.m[2][1], 0.0,
		matEyeRight.m[0][2], matEyeRight.m[1][2], matEyeRight.m[2][2], 0.0,
		matEyeRight.m[0][3], matEyeRight.m[1][3], matEyeRight.m[2][3], 1.0f
	);

	return glm::inverse(matrixObj);
}


//-----------------------------------------------------------------------------
// Purpose: Gets a Current View Projection Matrix with respect to nEye,
//          which may be an Eye_Left or an Eye_Right.
//-----------------------------------------------------------------------------
glm::mat4 VRInputs::GetPV(vr::Hmd_Eye nEye)
{
	glm::mat4 matMVP;

	if (nEye == vr::Eye_Left)
	{
		matMVP = m_mat4ProjectionLeft * m_mat4eyePosLeft * m_mat4HMDPose;
	}
	else
	{
		matMVP = m_mat4ProjectionRight * m_mat4eyePosRight * m_mat4HMDPose;
	}

	return matMVP;
}

glm::mat4 VRInputs::GetV(vr::Hmd_Eye nEye)
{
	if (nEye == vr::Eye_Left)
	{
		return m_mat4eyePosLeft * m_mat4HMDPose;
	}
	else// if (nEye == vr::Eye_Right)
	{
		return m_mat4eyePosRight * m_mat4HMDPose;
	}
}

bool VRInputs::SetMatrix(const Pkt::Cl_Input& _pkt)
{
	switch (_pkt.InputType)
	{
	case Pkt::InputTypes::MATRIX_HMD_POSE:
		{
			std::lock_guard<std::mutex> guard(m_inputMatrixLock);
			if ((long long)_pkt.TimeStamp > (long long)m_matrixIndex)
			{

				memcpy(&m_mat4HMDPose, _pkt.Data, sizeof(glm::mat4));
				m_currentTime = _pkt.ClockTime;
				m_matrixIndex = _pkt.TimeStamp;
				//printf("Got input %d\n", m_matrixIndex);

				{
					std::lock_guard<std::mutex> guard(m_runMutex);
					m_inputIsFresh = true;
					m_inputCondition.notify_one();
				}

				//long long delay = Timers::Get().Time() - _pkt.ClockTime - Timers::Get().ServerDiff();

				//printf("Input delay %lld %lld\n", delay, Timers::Get().ServerDiff());

				//printf("got new input %lld\n", m_matrixIndex);
			}
			else
			{
				return false;
			}
		}
		break;
	case Pkt::InputTypes::MATRIX_EYE_POS_LEFT:
		printf("MATRIX_EYE_POS_LEFT \n");
		memcpy(&m_mat4eyePosLeft, _pkt.Data, sizeof(glm::mat4));
		break;
	case Pkt::InputTypes::MATRIX_EYE_POS_RIGHT:
		printf("MATRIX_EYE_POS_RIGHT \n");
		memcpy(&m_mat4eyePosRight, _pkt.Data, sizeof(glm::mat4));
		break;
	case Pkt::InputTypes::MATRIX_EYE_PROJ_LEFT:
		printf("MATRIX_EYE_PROJ_LEFT \n");
		memcpy(&m_mat4ProjectionLeft, _pkt.Data, sizeof(glm::mat4));
		break;
	case Pkt::InputTypes::MATRIX_EYE_PROJ_RIGHT:
		printf("MATRIX_EYE_PROJ_RIGHT \n");
		memcpy(&m_mat4ProjectionRight, _pkt.Data, sizeof(glm::mat4));
		break;
	default:
		break;
	}
	return true;
}

bool VRInputs::HasNewInput()
{
	std::lock_guard<std::mutex> guard(m_inputMatrixLock);

	if (m_matrixIndex != m_drawnMatrixIndex)
	{
		//printf("accepted input %lld\n", m_matrixIndex);
		m_drawnMatrixIndex = m_matrixIndex;
		return true;
	}

	return false;
}

long long VRInputs::GetMatrixIndex()
{
	return m_matrixIndex;
}

void VRInputs::InputCondWait()
{
	std::unique_lock<std::mutex> waitLock(m_runMutex);
	if (m_inputCondition.wait_for(waitLock, std::chrono::microseconds(100), [this] { return m_inputIsFresh; }))
	{
		m_inputIsFresh = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const glm::mat4& VRInputs::UpdateHMDMatrixPose(float _dt, int _predict)
{
	if (m_pHMD == NULL)
	{
		//Running without HMD, view matrix is updated by mouse and keyboard and sent in the same way.

		if (MOUSE_LOOK)
		{
			//hacky mouse-movement that will reach limits to the left and right because mousepos is not re-centered. But works good enough for this

			int x, y;
			SDL_GetMouseState(&x, &y);

			float dx = float(m_prevMouseX - x) * 0.01f;
			float dy = float(m_prevMouseY - y) * 0.01f;

			//printf("%f, %f\n", dx, dy);

			m_camera->Yaw(dx);
			m_camera->Pitch(dy);

			m_prevMouseX = x;
			m_prevMouseY = y;

			//m_mat4HMDPose[5] = -1;
		}

		memcpy(&m_mat4HMDPose[0], &m_camera->GetView()[0], sizeof(float) * 16);

		//Experiments
		if (ROTATING_VIEW)
		{
			ArtificialRotatingView(_dt);
		}

		return m_mat4HMDPose;
	}

	//blocking, this is the sync-point of the program
	vr::VRCompositor()->WaitGetPoses(m_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);
	
	for (int nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; ++nDevice)
	{
		if (m_rTrackedDevicePose[nDevice].bPoseIsValid)
		{
			m_rmat4DevicePose[nDevice] = ConvertSteamVRMatrixToglm(m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking);
		}
	}

	if (m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
	{
		m_mat4HMDPose = m_rmat4DevicePose[vr::k_unTrackedDeviceIndex_Hmd];

		//Predict future poses if running with delay
		if (_predict)
		{
			// for somebody asking for the default figure out the time from now to photons.
			float fSecondsSinceLastVsync;
			vr::VRSystem()->GetTimeSinceLastVsync(&fSecondsSinceLastVsync, NULL);

			float fDisplayFrequency = vr::VRSystem()->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_DisplayFrequency_Float);
			float fFrameDuration = 1.f / fDisplayFrequency;
			float fVsyncToPhotons = vr::VRSystem()->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SecondsFromVsyncToPhotons_Float);

			float fPredictedSecondsFromNow = fFrameDuration - fSecondsSinceLastVsync + fVsyncToPhotons;

			fPredictedSecondsFromNow += (_predict-1) * fFrameDuration;

			//printf("Predicting ahead %fs\n", fPredictedSecondsFromNow);

			vr::VRSystem()->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding, fPredictedSecondsFromNow, &m_trackedDevicePose, 1);

			m_mat4HMDPose = ConvertSteamVRMatrixToglm(m_trackedDevicePose.mDeviceToAbsoluteTracking);
		}

		//Experiments
		if (ROTATING_VIEW)
		{
			ArtificialRotatingView(_dt);
		}

		if (ROTATION_FROM_FILE_TEST)
		{
			if (m_fileRotationIndex < m_fileRotations.size())
			{
				glm::vec3& rotation = m_fileRotations[m_fileRotationIndex];

				m_mat4HMDPose = glm::mat4(1);
				m_mat4HMDPose = 
					glm::translate(glm::vec3(0, 1, 0)) * 
					glm::rotate(rotation.z, glm::vec3(0, 1, 0)) *
					glm::rotate(rotation.y, glm::vec3(1, 0, 0)) * 
					glm::rotate(rotation.x, glm::vec3(0, 0, 1));

				m_fileRotationIndex++;
			}
			else
			{
				printf("Rotation test finished\n");
			}
		}

		m_mat4HMDPose = glm::inverse(m_mat4HMDPose);
	}
	else
	{
		//This is a tracking error
		//m_dataRecorder.g_QoEData.TrackingErrors++;
	}

	return m_mat4HMDPose;
}

void VRInputs::ArtificialRotatingView(float _dt)
{
	static int direction = 1;

	_dt = 1.0f / (float)StatCalc::GetParameter("FRAME_RATE");

	m_simulatedLookAngle += _dt * direction * (float)StatCalc::GetParameter("ROTATING_DEG_PER_SEC");

	float limit = 360;

	if (m_simulatedLookAngle >= limit)
	{
		m_simulatedLookAngle = 0;
		//direction = -1;
	}
	/*else if (m_simulatedLookAngle < -limit)
	{
		direction = 1;
	}*/

	m_mat4HMDPose = glm::rotate(glm::radians(m_simulatedLookAngle), glm::vec3(0, 1, 0)) * m_mat4HMDPose;
}

void VRInputs::SetRotateViewOnOff(bool _on)
{
	ROTATING_VIEW = _on;
}

//-----------------------------------------------------------------------------
// Purpose: Converts a SteamVR matrix to our local matrix class
//-----------------------------------------------------------------------------
glm::mat4 VRInputs::ConvertSteamVRMatrixToglm(const vr::HmdMatrix34_t &matPose)
{
	glm::mat4 matrixObj(
		matPose.m[0][0], matPose.m[1][0], matPose.m[2][0], 0.0,
		matPose.m[0][1], matPose.m[1][1], matPose.m[2][1], 0.0,
		matPose.m[0][2], matPose.m[1][2], matPose.m[2][2], 0.0,
		matPose.m[0][3], matPose.m[1][3], matPose.m[2][3], 1.0f
	);
	return matrixObj;
}

void VRInputs::SetHMDPose(glm::mat4 _mat4)
{

	m_mat4HMDPose = _mat4;
}

const std::vector<VertexDataLens>& VRInputs::GetLensVector()
{
	return m_lensVertices; 
}

void VRInputs::AddLensVertex(VertexDataLens _input)
{
	m_lensVertices.push_back(_input);
}

void VRInputs::ButtonPress(int _key)
{
	float speed = 0.1f;

	if (_key == SDLK_w)
	{
		m_camera->Move(-speed);
	}
	else if (_key == SDLK_s)
	{
		m_camera->Move(speed);
	}
	else if (_key == SDLK_d)
	{
		m_camera->Strafe(speed);
	}
	else if (_key == SDLK_a)
	{
		m_camera->Strafe(-speed);
	}
}

glm::vec4 VRInputs::GetCameraPosV4()
{
	glm::vec4 pos;

	pos.x = m_camera->GetPosition().x;
	pos.y = m_camera->GetPosition().y;
	pos.z = m_camera->GetPosition().z;
	pos.w = 1;

	return pos;
}

bool VRInputs::IsRotationTestComplete()
{
	return m_fileRotationIndex >= 65000;
	//return m_fileRotationIndex >= m_fileRotations.size();
}

const vr::HmdMatrix34_t& VRInputs::GetHMDPoseUsedInPrediction()
{
	return m_trackedDevicePose.mDeviceToAbsoluteTracking;
}