#pragma once
#include <glm.hpp>
#include "GL/glew.h"
#include <GLShaderHandler.h>
#include "RenderHelper.h"

class CompressorMap
{

public:
	struct Pixel
	{
		unsigned short X;
		unsigned short Y;
	};

	CompressorMap();
	~CompressorMap();

	void LoadMaps();
	void LoadShader();
	void Render(int _eye, GLuint fromTexture);

	GLuint GetEyeTexture(int _eye);
	const glm::ivec2& GetCompressedSize();

private:
	void MakeTexture(int _eye, FILE* _file);


	glm::ivec2 m_encodedSize;
	glm::ivec2 m_decodedSize;
	ShaderHandler* m_shader=nullptr;

	FramebufferDesc m_eye[2];
	GLuint m_compressionTexture[2] = { 0,0 };
};

