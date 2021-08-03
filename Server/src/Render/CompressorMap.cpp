#include "CompressorMap.h"
#include <stdio.h>
#include <cuda_gl_interop.h> //includes GL.h
#include "RenderHelper.h"
#include "StatCalc.h"

CompressorMap::CompressorMap()
{

}

CompressorMap::~CompressorMap()
{
	glDeleteTextures(2, m_compressionTexture);
	delete m_shader;
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

	//TODO: TEMP, make part of file
	//m_compressedSize.x = 1232;
	//m_compressedSize.y = 1680;

// 	m_size.x = 1520;
// 	m_size.y = 1360;

	CreateFrameBuffer(m_encodedSize.x, m_encodedSize.y, m_eye[0], true);
	CreateFrameBuffer(m_encodedSize.x, m_encodedSize.y, m_eye[1], true);

	LoadShader();
}

const glm::ivec2& CompressorMap::GetCompressedSize()
{
	return m_encodedSize;
}

void CompressorMap::LoadShader()
{
	if (m_shader != nullptr)
		delete m_shader;

	m_shader = new ShaderHandler();

	m_shader->CreateShaderProgram();
	m_shader->AddShader("../content/shaders/fsCompress.frag", GL_FRAGMENT_SHADER);
	m_shader->AddShader("../content/shaders/vsCompress.vert", GL_VERTEX_SHADER);
	m_shader->LinkShaderProgram();

	m_shader->AddUniform("gHeight");
	
}

void CompressorMap::Render(int _eye, GLuint fromTexture)
{
	//This function assumes a 2D quad is already bound as VAO
	glBindFramebuffer(GL_FRAMEBUFFER, m_eye[_eye].m_nResolveFramebufferId); //TO

	glClear(GL_COLOR_BUFFER_BIT);

	m_shader->UseProgram();

	m_shader->SetUniformV_loc("gHeight", m_decodedSize.y);

	glViewport(0, 0, m_encodedSize.x, m_encodedSize.y);

	//m_shader->SetUniformV_loc("gWidth", m_size.x);
	//m_shader->SetUniformV_loc("gHeight", m_size.y);

	//glActiveTexture(GL_TEXTURE0);
	//glBindTexture(GL_TEXTURE_2D, fromTexture);
	glBindImageTexture(0, fromTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
	glBindImageTexture(1, m_compressionTexture[_eye], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RG16);
	//glBindImageTexture(2, GetEyeTexture(_eye), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);

	glActiveTexture(GL_TEXTURE0);
	glUseProgram(0);

	GLErrorCheck(__FUNCTION__,__LINE__);
}

GLuint CompressorMap::GetEyeTexture(int _eye)
{
	return m_eye[_eye].m_nResolveTextureId;
}