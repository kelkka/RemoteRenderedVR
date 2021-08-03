/*
* 
*/

//Just an old camera class for use when testing without vr headset

#pragma once

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/transform.hpp>

class GLCamera
{
private:

	int m_viewPort;
    int m_width;
	int m_height;
	int m_Mapwidth;
	int m_Mapheight;
	
	float				m_FOVy, m_aspectRatio, m_nearZ, m_farZ;
	glm::vec3			m_position, m_forward, m_right, m_up;
	glm::mat4                       m_projection,m_view;
	void*                           m_vpProjection;
	void*                           m_vpView;
	void UpdateView();
	void UpdateProjection();


public:

	GLCamera(void);
	GLCamera(float fovy, int width,int height, float nearZ, float farZ);
	~GLCamera(void);

	glm::mat4* GetView();
	glm::mat4* GetProjection();

	glm::vec3 GetPosition()		{ return m_position; }
	glm::vec3 GetForward()		{ return m_forward; }
	glm::vec3 GetRight()		{ return m_right; }
	glm::vec3 GetUp()			{ return m_up; }

	void* GetViewPort() { return &m_viewPort; }

	void Move(glm::vec3 _move);
	void Move(float _move);
	void MoveOrtho(float _move);
	void SetPosition(glm::vec3 position);
	void SetPosition(glm::vec2 position);
	void SetForward(glm::vec3 forward);
	void SetLookAt(glm::vec3 _target);
	void Strafe(float delta);
	void StrafeOrtho(float delta);
	void Yaw(float angle);
	void Pitch(float angle);
	void Roll(float angle);
	void RotateY(float angle);

	glm::vec2 GetCoordInCameraSpace(const glm::vec2 &_pos);
};