/*
* 
*/

#include "GLCamera.h"
#include <string>
#include <GL/glew.h>
#include <ext/matrix_clip_space.hpp>

GLCamera::GLCamera(float _fovy, int _width,int _height, float _nearZ, float _farZ)
{
    m_width = _width;
    m_height = _height;
    
    m_aspectRatio = (float)m_width / (float)m_height;

    m_FOVy = _fovy;

    m_nearZ = _nearZ;
    m_farZ = _farZ;

    m_position  = glm::vec3(0, 1, 0);
    m_forward   = glm::vec3(0, 0, 1);
    m_right     = glm::vec3(1, 0, 0);
    m_up        = glm::vec3(0, 1, 0);

    UpdateView();
    UpdateProjection();
    SetForward(m_forward);
}

GLCamera::GLCamera(void)
{

}

GLCamera::~GLCamera(void)
{

}


void GLCamera::UpdateView()
{
    glm::vec3 R = m_right;
	glm::vec3 U = m_up;
	glm::vec3 F = m_forward;
	glm::vec3 P = m_position;

	// Keep camera's axes orthogonal to each other and of unit length.
	F =  glm::normalize(F);
	U =  glm::normalize(glm::cross(F, R));
	// U, L already ortho-normal, so no need to normalize cross product.
    R = glm::cross(U, F);

	// Fill in the view matrix entries.
	float x = -glm::dot(P, R);
	float y = -glm::dot(P, U);
	float z = -glm::dot(P, F);

	m_view[0][0] = R.x;
	m_view[1][0] = R.y;
	m_view[2][0] = R.z;
	m_view[3][0] = x;

	m_view[0][1] = U.x;
	m_view[1][1] = U.y;
	m_view[2][1] = U.z;
	m_view[3][1] = y;

	m_view[0][2] = F.x;
	m_view[1][2] = F.y;
	m_view[2][2] = F.z;
	m_view[3][2] = z;

	m_view[0][3] = 0.0f;
	m_view[1][3] = 0.0f;
	m_view[2][3] = 0.0f;
	m_view[3][3] = 1.0f;
 }

void GLCamera::UpdateProjection()
{

    printf("FOV: %f | AspectRatio: %f | nearZ: %f | farZ: %f\n",m_FOVy,m_aspectRatio,m_nearZ,m_farZ);

	static const float n = 0.1f;
	static const float f = 150.0f;
	static const float r = 1080.0f / 1200.0f;
	static const float a = glm::radians<float>(90);// 0.6f * glm::pi<float>();
	
	static const glm::mat4 P =
	{
		1 / (r * tan(a / 2.0f)),	0,						0,					0,
		0,							1 / (tan(a / 2.0f)),	0,					0,
		0,							0,						f / (f - n),		1,
		0,							0,						-(f * n) / (f - n),	0
	};

	m_projection = P;


	//TODO IF D3D
	//m_projection = glm::perspectiveFovLH(m_FOVy, (float)m_width, (float)m_height, m_nearZ, m_farZ);
	//m_projection = glm::perspectiveFovRH(m_FOVy, (float)m_width, (float)m_height, m_nearZ, m_farZ);

}

glm::mat4 *GLCamera::GetProjection()
{
    return &m_projection;
}

glm::mat4* GLCamera::GetView()
{
    return &m_view;
}

void GLCamera::SetLookAt(glm::vec3 _target)
{
    glm::vec3 forward;
    forward.x = _target.x - m_position.x;
    forward.y = _target.y - m_position.y;
    forward.z = _target.z - m_position.z;

    SetForward(forward);
}

void GLCamera::SetForward(glm::vec3 forward)
{
	glm::vec3 pos = glm::vec3(m_position.x,m_position.y,m_position.z);
	glm::vec3 direction = glm::vec3(forward.x,forward.y,forward.z);
	direction = glm::normalize(direction);
	glm::vec3 up2 = glm::vec3(0, 1, 0);

    if (direction == glm::vec3(0,0,0))
		direction = glm::vec3(0, 0, 1);

	else if (direction == glm::vec3(0, -1, 0))
		up2 = glm::vec3(0, 0, 1);

	else if (direction == glm::vec3(0,1,0))
		up2 = glm::vec3(0, 0, -1);

        
	m_view = glm::lookAt(pos,pos+direction,up2);

	m_right = glm::vec3(m_view[0][0], m_view[1][0], m_view[2][0]);
	m_up = glm::vec3(m_view[0][1], m_view[1][1], m_view[2][1]);
	m_forward = glm::vec3(m_view[0][2], m_view[1][2], m_view[2][2]);
    

        
	UpdateView();
        
}

void GLCamera::Move(glm::vec3 _move)
{
    m_position.x += _move.x;
    m_position.y += _move.y;
    m_position.z += _move.z;
    
    UpdateView();
    
}

void GLCamera::Move(float _move)
{
    m_position.x += (m_forward.x)*_move;
    m_position.y += (m_forward.y)*_move;
    m_position.z += (m_forward.z)*_move;
    
    UpdateView();
    
}

void GLCamera::MoveOrtho(float _move)
{
	m_position.y += _move;

	UpdateView();
}

void GLCamera::SetPosition(glm::vec3 position)
{
	m_position = position;
	UpdateView();
}

void GLCamera::SetPosition(glm::vec2 position)
{
	m_position.x = position.x;
	m_position.y = position.y;

	UpdateView();
}

void GLCamera::Strafe(float delta)
{
	m_position += delta * m_right;

	UpdateView();
}

void GLCamera::StrafeOrtho(float delta)
{
	m_position.x += delta;

	UpdateView();
}

void GLCamera::Roll(float angle) 
{
	glm::mat4 R = glm::rotate(angle, m_forward);
	glm::vec4 r = (R * glm::vec4(m_right, 0));
	m_right.x = r.x;
	m_right.y = r.y;
	m_right.z = r.z;
	glm::vec4 u = (R * glm::vec4(m_up, 0));
	m_up.x = u.x;
	m_up.y = u.y;
	m_up.z = u.z;

	UpdateView();
}

void GLCamera::Pitch(float angle) 
{
	glm::mat4 R = glm::rotate(angle, m_right);
	glm::vec4 l = (R * glm::vec4(m_forward, 0));
	m_forward.x = l.x;
	m_forward.y = l.y;
	m_forward.z = l.z;
	glm::vec4 u = (R * glm::vec4(m_up, 0));
	m_up.x = u.x;
	m_up.y = u.y;
	m_up.z = u.z;

	UpdateView();
}

void GLCamera::Yaw(float angle) 
{
	glm::mat4 R = glm::rotate(angle, glm::vec3(0, 1, 0));
	glm::vec4 l = (R * glm::vec4(m_forward, 0));
	m_forward.x = l.x;
	m_forward.y = l.y;
	m_forward.z = l.z;
	glm::vec4 r = (R * glm::vec4(m_right, 0));
	m_right.x = r.x;
	m_right.y = r.y;
	m_right.z = r.z;

	UpdateView();
}

void GLCamera::RotateY(float angle) 
{   
	glm::mat4 R = glm::rotate(angle, glm::vec3(0, 1, 0));
	glm::vec4 l = (glm::vec4(m_forward, 0) * R);
	m_forward.x = l.x;
	m_forward.y = l.y;
	m_forward.z = l.z;
	glm::vec4 r = (glm::vec4(m_right, 0) * R);
	m_right.x = r.x;
	m_right.y = r.y;
	m_right.z = r.z;
	glm::vec4 u = (glm::vec4(m_up, 0) * R);
	m_up.x = u.x;
	m_up.y = u.y;
	m_up.z = u.z;

	UpdateView();
}

glm::vec2 GLCamera::GetCoordInCameraSpace(const glm::vec2 &_pos)
{
	glm::vec4 worldPos;
	worldPos.x = _pos.x;
	worldPos.y = _pos.y;
	worldPos.z = 0;
	worldPos.w = 1;

	worldPos = m_projection * m_view* worldPos;

	glm::vec3 ndcSpacePos = glm::vec3(worldPos.x, worldPos.y, worldPos.z) / worldPos.w;

	glm::vec2 windowSpacePos = ((glm::vec2(ndcSpacePos.x, ndcSpacePos.y) + 1.0f) / 2.0f) * glm::vec2(m_width,m_height);

	return windowSpacePos;
}