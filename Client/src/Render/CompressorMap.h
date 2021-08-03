#pragma once
#include <glm.hpp>
#include "GL/glew.h"
#include <GLShaderHandler.h>

//This is for Hidden Area Mesh Remapping

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
	GLuint GetCompressionTexture(int _eye);

	const glm::ivec2& GetCompressedSize();
private:

	void MakeTexture(int _eye, FILE* _file);

	GLuint m_compressionTexture[2] = { 0,0 };

	glm::ivec2 m_encodedSize;
	glm::ivec2 m_decodedSize;
};

