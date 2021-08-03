#include "CompressorMap.h"
#include <stdio.h>
#include <cuda_gl_interop.h> //includes GL.h
#include "StatCalc.h"

CompressorMap::CompressorMap()
{

}

CompressorMap::~CompressorMap()
{
	glDeleteTextures(2, m_compressionTexture);
}

void CompressorMap::LoadMaps()
{
	std::string pathL = StatCalc::GetParameterStr("LENS_MAPPING_PATH");
	pathL += "_left.map";
	std::string pathR = StatCalc::GetParameterStr("LENS_MAPPING_PATH");
	pathR += "_right.map";


	FILE* input;

	if (fopen_s(&input, pathL.c_str(), "rb") != 0)
	{
		printf("Failed to read encoder map %s\n", pathL.c_str());
		return;
	}

	glGenTextures(2, m_compressionTexture);

	MakeTexture(0, input);

	fclose(input);

	if (fopen_s(&input, pathR.c_str(), "rb") != 0)
	{
		printf("Failed to read encoder map %s\n", pathR.c_str());
		return;
	}

	MakeTexture(1, input);

	fclose(input);
}

void CompressorMap::MakeTexture(int _eye, FILE* _file)
{
	//The encoded size, will be overwritten by the second eye but should be the same anyway
	fread(&m_encodedSize, sizeof(glm::ivec2), 1, _file);
	//The decoded size
	fread(&m_decodedSize, sizeof(glm::ivec2), 1, _file);
	Pixel* map = (Pixel*)malloc(m_decodedSize.x * m_decodedSize.y * sizeof(Pixel));
	fread(map, sizeof(Pixel), m_decodedSize.x * m_decodedSize.y, _file);

	printf("Read %d,%d compression map\n", m_decodedSize.x, m_decodedSize.y);

	glBindTexture(GL_TEXTURE_2D, m_compressionTexture[_eye]);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16UI, m_decodedSize.x, m_decodedSize.y, 0, GL_RG_INTEGER, GL_UNSIGNED_SHORT, map);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, 0);

	free(map);
}

GLuint CompressorMap::GetCompressionTexture(int _eye)
{
	return m_compressionTexture[_eye];
}

const glm::ivec2& CompressorMap::GetCompressedSize()
{
	return m_encodedSize;
}