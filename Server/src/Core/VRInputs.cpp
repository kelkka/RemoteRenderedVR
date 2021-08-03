#include "VRInputs.h"
#include <gtc/matrix_transform.hpp>
#include "../Utility/Timers.h"

#include <SDL.h>

VRInputs::VRInputs()
{

}

VRInputs::~VRInputs()
{

}


// Gets a Current View Projection Matrix with respect to nEye,
// which may be an Eye_Left or an Eye_Right.

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

glm::mat4 VRInputs::GetP(vr::Hmd_Eye nEye)
{
	if (nEye == vr::Eye_Left)
	{
		return m_mat4ProjectionLeft;
	}
	else// if (nEye == vr::Eye_Right)
	{
		return m_mat4ProjectionRight;
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

void VRInputs::GetHMDWorld(glm::mat4& inverted)
{
	inverted = glm::inverse(m_mat4HMDPose);
}

HiddenAreaMesh& VRInputs::AllocateHiddenAreaMesh(int _eye, unsigned int _triangleCount)
{
	if (m_hiddenAreaMesh[_eye].Allocate(_triangleCount) != 0)
	{
		printf("Error allocating hidden area mesh\n");
	}
	return m_hiddenAreaMesh[_eye];
}

const HiddenAreaMesh& VRInputs::GetHiddenAreaMesh(int _eye)
{
	if (m_hiddenAreaMesh->VertexData == nullptr)
	{
		printf("HiddenAreaMesh not allocated yet!\n");
	}

	return m_hiddenAreaMesh[_eye];
}
