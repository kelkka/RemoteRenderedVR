#pragma once
#include <glm/glm.hpp>

struct VertexDataLens
{
	glm::vec2 position = glm::vec2();
	glm::vec2 texCoordRed = glm::vec2();
	glm::vec2 texCoordGreen = glm::vec2();
	glm::vec2 texCoordBlue = glm::vec2();
};

struct HiddenAreaMesh
{
	glm::vec2* VertexData = nullptr;
	unsigned int TriangleCount = 0;

	int Allocate(unsigned int _triangleCount)
	{
		TriangleCount = _triangleCount;

		if (VertexData == nullptr)
		{
			VertexData = (glm::vec2*)malloc(sizeof(glm::vec2) * _triangleCount * 3);
		}
		else
		{
			printf("Invalid usage of AllocateHiddenAreaMesh!\n");
			return -1;
		}

		return 0;
	}

	~HiddenAreaMesh()
	{
		if (VertexData != nullptr)
		{
			free(VertexData);
			VertexData = nullptr;
		}
	}
};