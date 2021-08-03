#pragma once
#include <string>
#include <openvr.h>

//#include "shared/Matrices.h"
#include <glm.hpp>
#include <gtc/quaternion.hpp>
#include <Packets.h>
#include <mutex>
#include <vector>
#include <StatCalc.h>

//Input class, handles HMD pose or mouse and keyboard movement

class VRInputs
{
public:
	VRInputs();
	~VRInputs();

	glm::mat4 GetPV(vr::Hmd_Eye nEye);

	glm::mat4 GetV(vr::Hmd_Eye nEye);

	glm::mat4 GetP(vr::Hmd_Eye nEye);
	bool SetMatrix(const Pkt::Cl_Input& _pkt);

	bool HasNewInput();

	long long GetMatrixIndex();

	void InputCondWait();

	void SetHMDPose(glm::mat4 _mat4);

	long long GetClockTime() { return m_currentTime; }

	const std::vector<VertexDataLens> & GetLensVector();

	void AddLensVertex(VertexDataLens _input);

	bool IsLensDataOK() { return m_lensDataOK; }
	void SetLensDataOK() { m_lensDataOK = true; }

	void GetHMDWorld(glm::mat4& inverted);

	HiddenAreaMesh& AllocateHiddenAreaMesh(int _eye, unsigned int _triangleCount);
	const HiddenAreaMesh& GetHiddenAreaMesh(int _eye);

private:

	glm::mat4 m_mat4HMDPose;
	glm::mat4 m_mat4eyePosLeft;
	glm::mat4 m_mat4eyePosRight;

	glm::mat4 m_mat4ProjectionLeft;
	glm::mat4 m_mat4ProjectionRight;

	long long m_currentTime=0;
	volatile long long m_matrixIndex = -1;
	volatile long long m_drawnMatrixIndex = -1;

	std::mutex m_inputMatrixLock;

	std::vector<VertexDataLens> m_lensVertices;

	bool m_lensDataOK = false;

	bool m_inputIsFresh = false;
	std::mutex m_runMutex;
	std::condition_variable m_inputCondition;

	HiddenAreaMesh m_hiddenAreaMesh[2];
};