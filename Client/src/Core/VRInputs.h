/*
* 
*/

//Input class, handles HMD pose or mouse and keyboard movement

#pragma once
#include <string>
#include <openvr.h>

#include <glm.hpp>
#include <gtc/quaternion.hpp>
#include <Packets.h>
#include <mutex>
#include <vector>
#include "StatCalc.h"

#include <GLCamera.h>

class VRInputs
{
public:
	VRInputs(vr::IVRSystem * _hmd);
	~VRInputs();

	void SetupCameras(int _w, int _h);

	void LoadRotationTestFile();

	glm::mat4 GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye, int _w, int _h);

	glm::mat4 GetHMDMatrixPoseEye(vr::Hmd_Eye nEye);

	glm::mat4 GetPV(vr::Hmd_Eye nEye);

	glm::mat4 GetV(vr::Hmd_Eye nEye);

	bool SetMatrix(const Pkt::Cl_Input & _pkt);

	bool HasNewInput();

	long long GetMatrixIndex();

	void InputCondWait();

	const glm::mat4& UpdateHMDMatrixPose(float _dt, int _predict);

	void ArtificialRotatingView(float _dt);

	void SetRotateViewOnOff(bool _on);
	static glm::mat4 ConvertSteamVRMatrixToglm(const vr::HmdMatrix34_t & matPose);
	void SetHMDPose(glm::mat4 _mat4);

	long long GetClockTime() { return m_currentTime; }

	const std::vector<VertexDataLens> & GetLensVector();

	void AddLensVertex(VertexDataLens _input);

	bool IsLensDataOK() { return m_lensDataOK; }
	void SetLensDataOK() { m_lensDataOK = true; }

	void ButtonPress(int _key);

	glm::vec4 GetCameraPosV4();

	const vr::HmdMatrix34_t& VRInputs::GetHMDPoseUsedInPrediction();

	bool IsRotationTestComplete();

private:

	glm::mat4 m_mat4HMDPose;
	glm::mat4 m_mat4eyePosLeft;
	glm::mat4 m_mat4eyePosRight;

	glm::mat4 m_mat4ProjectionCenter;
	glm::mat4 m_mat4ProjectionLeft;
	glm::mat4 m_mat4ProjectionRight;
	vr::IVRSystem* m_pHMD;

	vr::TrackedDevicePose_t m_rTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
	glm::mat4 m_rmat4DevicePose[vr::k_unMaxTrackedDeviceCount];

	long long m_currentTime=0;
	volatile long long m_matrixIndex = -1;
	volatile long long m_drawnMatrixIndex = -1;

	std::mutex m_inputMatrixLock;

	std::vector<VertexDataLens> m_lensVertices;

	bool m_lensDataOK = false;

	bool m_inputIsFresh = false;
	std::mutex m_runMutex;
	std::condition_variable m_inputCondition;
	GLCamera* m_camera = nullptr;


	int m_prevMouseX = 0;
	int m_prevMouseY = 0;

	float m_simulatedLookAngle = 0;


	vr::TrackedDevicePose_t m_trackedDevicePose;

	std::vector<glm::vec3> m_fileRotations;
	int m_fileRotationIndex = 0;

	//Config values

	bool MOUSE_LOOK = false;
	bool ROTATING_VIEW = false;
	bool ROTATION_FROM_FILE_TEST = false;

};