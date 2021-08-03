#pragma once
#include <glm.hpp>
#include "GL/glew.h"
#include <vector>

const float CUBE_SIZE = 3;

struct FramebufferDesc
{
	GLuint m_nDepthBufferId = 0;
	GLuint m_nRenderTextureId = 0;
	GLuint m_nRenderFramebufferId = 0;
	GLuint m_nResolveTextureId = 0;
	GLuint m_nResolveFramebufferId = 0;
	glm::ivec2 m_size = glm::ivec2();
};

bool CreateFrameBuffer(int nWidth, int nHeight, FramebufferDesc& framebufferDesc, bool resolveOnly);
void GLErrorCheck(const char* _method, int _line);
void AddCubeVertex(float fl0, float fl1, float fl2, float fl3, float fl4, float n_x, float n_y, float n_z, std::vector<float>& vertdata);
void AddCubeMap(std::vector<float>& vertdata);
void AddCube(std::vector<float>& vertdata);
GLuint LoadTexture(const char* _path, GLint _texParameter);

