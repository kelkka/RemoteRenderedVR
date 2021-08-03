#include "RenderHelper.h"
#include <stdio.h>

#include "PNG.h"

//#define DO_GL_ERROR_CHECKS


//  Creates a frame buffer. Returns true if the buffer was set up.
//          Returns false if the setup failed.

bool CreateFrameBuffer(int nWidth, int nHeight, FramebufferDesc& framebufferDesc, bool resolveOnly)
{
	framebufferDesc.m_size.x = nWidth;
	framebufferDesc.m_size.y = nHeight;

	if (!resolveOnly)
	{
		glGenFramebuffers(1, &framebufferDesc.m_nRenderFramebufferId);
		glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.m_nRenderFramebufferId);

		glGenRenderbuffers(1, &framebufferDesc.m_nDepthBufferId);
		glBindRenderbuffer(GL_RENDERBUFFER, framebufferDesc.m_nDepthBufferId);

		glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, nWidth, nHeight);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, framebufferDesc.m_nDepthBufferId);

		glGenTextures(1, &framebufferDesc.m_nRenderTextureId);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.m_nRenderTextureId);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, nWidth, nHeight, true);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.m_nRenderTextureId, 0);
	}

	glGenFramebuffers(1, &framebufferDesc.m_nResolveFramebufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.m_nResolveFramebufferId);

	glGenTextures(1, &framebufferDesc.m_nResolveTextureId);
	glBindTexture(GL_TEXTURE_2D, framebufferDesc.m_nResolveTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferDesc.m_nResolveTextureId, 0);

	GLErrorCheck(__func__, __LINE__);

	// check FBO status
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		printf("Frame buffer failed %d\n", status);
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return true;
}

void GLErrorCheck(const char* _method, int _line)
{
#ifdef DO_GL_ERROR_CHECKS
	GLenum err = glGetError();
	if (err != GL_NO_ERROR)
		printf("%d %s %d\n", _line, _method, err);
#endif
}

void AddCubeVertex(float fl0, float fl1, float fl2, float fl3, float fl4, float n_x, float n_y, float n_z, std::vector<float>& vertdata)
{
	vertdata.push_back(fl0);
	vertdata.push_back(fl1);
	vertdata.push_back(fl2);
	vertdata.push_back(fl3);
	vertdata.push_back(fl4);
	vertdata.push_back(n_x);
	vertdata.push_back(n_y);
	vertdata.push_back(n_z);
}

void AddCubeMap(std::vector<float>& vertdata)
{
	float size = CUBE_SIZE;

	glm::vec4 A = glm::vec4(-size, -size, -size, size);
	glm::vec4 B = glm::vec4(size, -size, -size, size);
	glm::vec4 C = glm::vec4(size, size, -size, size);
	glm::vec4 D = glm::vec4(-size, size, -size, size);
	glm::vec4 E = glm::vec4(-size, -size, size, size);
	glm::vec4 F = glm::vec4(size, -size, size, size);
	glm::vec4 G = glm::vec4(size, size, size, size);
	glm::vec4 H = glm::vec4(-size, size, size, size);

	float x = 0;
	float y = 0;

	float t = 1.0f / 6.0f;

	// triangles instead of quads
	//AddCubeVertex(E.x, E.y, E.z, 0, 1, 0, 0, 1, vertdata); //top left  //Front
	//AddCubeVertex(F.x, F.y, F.z, 1, 1, 0, 0, 1, vertdata); //top right
	//AddCubeVertex(G.x, G.y, G.z, 1, 0, 0, 0, 1, vertdata); //bottom right
	//AddCubeVertex(G.x, G.y, G.z, 1, 0, 0, 0, 1, vertdata); //bottom right
	//AddCubeVertex(H.x, H.y, H.z, 0, 0, 0, 0, 1, vertdata); //bottom left
	//AddCubeVertex(E.x, E.y, E.z, 0, 1, 0, 0, 1, vertdata); //top left

	//AddCubeVertex(B.x, B.y, B.z, 0, 1, 0, 0, -1, vertdata); //Back
	//AddCubeVertex(A.x, A.y, A.z, 1, 1, 0, 0, -1, vertdata);
	//AddCubeVertex(D.x, D.y, D.z, 1, 0, 0, 0, -1, vertdata);
	//AddCubeVertex(D.x, D.y, D.z, 1, 0, 0, 0, -1, vertdata);
	//AddCubeVertex(C.x, C.y, C.z, 0, 0, 0, 0, -1, vertdata);
	//AddCubeVertex(B.x, B.y, B.z, 0, 1, 0, 0, -1, vertdata);

	//AddCubeVertex(H.x, H.y, H.z, 0, 1, 0, 1, 0, vertdata); //Top
	//AddCubeVertex(G.x, G.y, G.z, 1, 1, 0, 1, 0, vertdata);
	//AddCubeVertex(C.x, C.y, C.z, 1, 0, 0, 1, 0, vertdata);
	//AddCubeVertex(C.x, C.y, C.z, 1, 0, 0, 1, 0, vertdata);
	//AddCubeVertex(D.x, D.y, D.z, 0, 0, 0, 1, 0, vertdata);
	//AddCubeVertex(H.x, H.y, H.z, 0, 1, 0, 1, 0, vertdata);

	//AddCubeVertex(A.x, A.y, A.z, 0, 1, 0, -1, 0, vertdata); //Bottom
	//AddCubeVertex(B.x, B.y, B.z, 1, 1, 0, -1, 0, vertdata);
	//AddCubeVertex(F.x, F.y, F.z, 1, 0, 0, -1, 0, vertdata);
	//AddCubeVertex(F.x, F.y, F.z, 1, 0, 0, -1, 0, vertdata);
	//AddCubeVertex(E.x, E.y, E.z, 0, 0, 0, -1, 0, vertdata);
	//AddCubeVertex(A.x, A.y, A.z, 0, 1, 0, -1, 0, vertdata);

	//AddCubeVertex(A.x, A.y, A.z, 0, 1, -1, 0, 0, vertdata); //Left
	//AddCubeVertex(E.x, E.y, E.z, 1, 1, -1, 0, 0, vertdata);
	//AddCubeVertex(H.x, H.y, H.z, 1, 0, -1, 0, 0, vertdata);
	//AddCubeVertex(H.x, H.y, H.z, 1, 0, -1, 0, 0, vertdata);
	//AddCubeVertex(D.x, D.y, D.z, 0, 0, -1, 0, 0, vertdata);
	//AddCubeVertex(A.x, A.y, A.z, 0, 1, -1, 0, 0, vertdata);

	//AddCubeVertex(F.x, F.y, F.z, 0, 1, 1, 0, 0, vertdata); //Right
	//AddCubeVertex(B.x, B.y, B.z, 1, 1, 1, 0, 0, vertdata);
	//AddCubeVertex(C.x, C.y, C.z, 1, 0, 1, 0, 0, vertdata);
	//AddCubeVertex(C.x, C.y, C.z, 1, 0, 1, 0, 0, vertdata);
	//AddCubeVertex(G.x, G.y, G.z, 0, 0, 1, 0, 0, vertdata);
	//AddCubeVertex(F.x, F.y, F.z, 0, 1, 1, 0, 0, vertdata);

	AddCubeVertex(E.x, E.y, E.z, 0, 0, 0, 0, 1, vertdata); //top left  //Front
	AddCubeVertex(F.x, F.y, F.z, 1, 0, 0, 0, 1, vertdata); //top right
	AddCubeVertex(G.x, G.y, G.z, 1, t, 0, 0, 1, vertdata); //bottom right
	AddCubeVertex(G.x, G.y, G.z, 1, t, 0, 0, 1, vertdata); //bottom right
	AddCubeVertex(H.x, H.y, H.z, 0, t, 0, 0, 1, vertdata); //bottom left
	AddCubeVertex(E.x, E.y, E.z, 0, 0, 0, 0, 1, vertdata); //top left

	AddCubeVertex(B.x, B.y, B.z, 0, t * 1, 0, 0, -1, vertdata); //top left  //Back
	AddCubeVertex(A.x, A.y, A.z, 1, t * 1, 0, 0, -1, vertdata); //top right
	AddCubeVertex(D.x, D.y, D.z, 1, t * 2, 0, 0, -1, vertdata); //bottom right
	AddCubeVertex(D.x, D.y, D.z, 1, t * 2, 0, 0, -1, vertdata); //bottom right
	AddCubeVertex(C.x, C.y, C.z, 0, t * 2, 0, 0, -1, vertdata); //bottom left
	AddCubeVertex(B.x, B.y, B.z, 0, t * 1, 0, 0, -1, vertdata); //top left

	AddCubeVertex(H.x, H.y, H.z, 0, t * 4, 0, 1, 0, vertdata); //Top
	AddCubeVertex(G.x, G.y, G.z, 1, t * 4, 0, 1, 0, vertdata);
	AddCubeVertex(C.x, C.y, C.z, 1, t * 3, 0, 1, 0, vertdata);
	AddCubeVertex(C.x, C.y, C.z, 1, t * 3, 0, 1, 0, vertdata);
	AddCubeVertex(D.x, D.y, D.z, 0, t * 3, 0, 1, 0, vertdata);
	AddCubeVertex(H.x, H.y, H.z, 0, t * 4, 0, 1, 0, vertdata);

	AddCubeVertex(A.x, A.y, A.z, 1, t * 3, 0, -1, 0, vertdata); //Bottom
	AddCubeVertex(B.x, B.y, B.z, 0, t * 3, 0, -1, 0, vertdata);
	AddCubeVertex(F.x, F.y, F.z, 0, t * 2, 0, -1, 0, vertdata);
	AddCubeVertex(F.x, F.y, F.z, 0, t * 2, 0, -1, 0, vertdata);
	AddCubeVertex(E.x, E.y, E.z, 1, t * 2, 0, -1, 0, vertdata);
	AddCubeVertex(A.x, A.y, A.z, 1, t * 3, 0, -1, 0, vertdata);

	AddCubeVertex(A.x, A.y, A.z, 0, t * 5, -1, 0, 0, vertdata); //Left
	AddCubeVertex(E.x, E.y, E.z, 1, t * 5, -1, 0, 0, vertdata);
	AddCubeVertex(H.x, H.y, H.z, 1, t * 6, -1, 0, 0, vertdata);
	AddCubeVertex(H.x, H.y, H.z, 1, t * 6, -1, 0, 0, vertdata);
	AddCubeVertex(D.x, D.y, D.z, 0, t * 6, -1, 0, 0, vertdata);
	AddCubeVertex(A.x, A.y, A.z, 0, t * 5, -1, 0, 0, vertdata);

	AddCubeVertex(F.x, F.y, F.z, 0, t * 4, 1, 0, 0, vertdata); //Right
	AddCubeVertex(B.x, B.y, B.z, 1, t * 4, 1, 0, 0, vertdata);
	AddCubeVertex(C.x, C.y, C.z, 1, t * 5, 1, 0, 0, vertdata);
	AddCubeVertex(C.x, C.y, C.z, 1, t * 5, 1, 0, 0, vertdata);
	AddCubeVertex(G.x, G.y, G.z, 0, t * 5, 1, 0, 0, vertdata);
	AddCubeVertex(F.x, F.y, F.z, 0, t * 4, 1, 0, 0, vertdata);
}

void AddCube(std::vector<float>& vertdata)
{
	float size = CUBE_SIZE;

	glm::vec4 A = glm::vec4(-size, -size, -size, size);
	glm::vec4 B = glm::vec4(size, -size, -size, size);
	glm::vec4 C = glm::vec4(size, size, -size, size);
	glm::vec4 D = glm::vec4(-size, size, -size, size);
	glm::vec4 E = glm::vec4(-size, -size, size, size);
	glm::vec4 F = glm::vec4(size, -size, size, size);
	glm::vec4 G = glm::vec4(size, size, size, size);
	glm::vec4 H = glm::vec4(-size, size, size, size);

	// triangles instead of quads
	AddCubeVertex(E.x, E.y, E.z, 0, 1, 0, 0, 1, vertdata); //Front
	AddCubeVertex(F.x, F.y, F.z, 1, 1, 0, 0, 1, vertdata);
	AddCubeVertex(G.x, G.y, G.z, 1, 0, 0, 0, 1, vertdata);
	AddCubeVertex(G.x, G.y, G.z, 1, 0, 0, 0, 1, vertdata);
	AddCubeVertex(H.x, H.y, H.z, 0, 0, 0, 0, 1, vertdata);
	AddCubeVertex(E.x, E.y, E.z, 0, 1, 0, 0, 1, vertdata);

	AddCubeVertex(B.x, B.y, B.z, 0, 1, 0, 0, -1, vertdata); //Back
	AddCubeVertex(A.x, A.y, A.z, 1, 1, 0, 0, -1, vertdata);
	AddCubeVertex(D.x, D.y, D.z, 1, 0, 0, 0, -1, vertdata);
	AddCubeVertex(D.x, D.y, D.z, 1, 0, 0, 0, -1, vertdata);
	AddCubeVertex(C.x, C.y, C.z, 0, 0, 0, 0, -1, vertdata);
	AddCubeVertex(B.x, B.y, B.z, 0, 1, 0, 0, -1, vertdata);

	AddCubeVertex(H.x, H.y, H.z, 0, 1, 0, 1, 0, vertdata); //Top
	AddCubeVertex(G.x, G.y, G.z, 1, 1, 0, 1, 0, vertdata);
	AddCubeVertex(C.x, C.y, C.z, 1, 0, 0, 1, 0, vertdata);
	AddCubeVertex(C.x, C.y, C.z, 1, 0, 0, 1, 0, vertdata);
	AddCubeVertex(D.x, D.y, D.z, 0, 0, 0, 1, 0, vertdata);
	AddCubeVertex(H.x, H.y, H.z, 0, 1, 0, 1, 0, vertdata);

	AddCubeVertex(A.x, A.y, A.z, 0, 1, 0, -1, 0, vertdata); //Bottom
	AddCubeVertex(B.x, B.y, B.z, 1, 1, 0, -1, 0, vertdata);
	AddCubeVertex(F.x, F.y, F.z, 1, 0, 0, -1, 0, vertdata);
	AddCubeVertex(F.x, F.y, F.z, 1, 0, 0, -1, 0, vertdata);
	AddCubeVertex(E.x, E.y, E.z, 0, 0, 0, -1, 0, vertdata);
	AddCubeVertex(A.x, A.y, A.z, 0, 1, 0, -1, 0, vertdata);

	AddCubeVertex(A.x, A.y, A.z, 0, 1, -1, 0, 0, vertdata); //Left
	AddCubeVertex(E.x, E.y, E.z, 1, 1, -1, 0, 0, vertdata);
	AddCubeVertex(H.x, H.y, H.z, 1, 0, -1, 0, 0, vertdata);
	AddCubeVertex(H.x, H.y, H.z, 1, 0, -1, 0, 0, vertdata);
	AddCubeVertex(D.x, D.y, D.z, 0, 0, -1, 0, 0, vertdata);
	AddCubeVertex(A.x, A.y, A.z, 0, 1, -1, 0, 0, vertdata);

	AddCubeVertex(F.x, F.y, F.z, 0, 1, 1, 0, 0, vertdata); //Right
	AddCubeVertex(B.x, B.y, B.z, 1, 1, 1, 0, 0, vertdata);
	AddCubeVertex(C.x, C.y, C.z, 1, 0, 1, 0, 0, vertdata);
	AddCubeVertex(C.x, C.y, C.z, 1, 0, 1, 0, 0, vertdata);
	AddCubeVertex(G.x, G.y, G.z, 0, 0, 1, 0, 0, vertdata);
	AddCubeVertex(F.x, F.y, F.z, 0, 1, 1, 0, 0, vertdata);
}


//  Loads PNG from file to use in game engine

GLuint LoadTexture(const char* _path, GLint _texParameter)
{
	int nImageWidth, nImageHeight, channels;

	unsigned char* imageRGBA = PNG::ReadPNG(_path, nImageWidth, nImageHeight, channels, 3);

	if (imageRGBA == 0)
		return -1;

	GLuint textureId = 0;

	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, nImageWidth, nImageHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, imageRGBA);

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _texParameter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _texParameter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);

	free(imageRGBA);

	return textureId;
}