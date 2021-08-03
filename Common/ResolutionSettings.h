#pragma once
#include <vector>
#include <glm.hpp>

class ResolutionSettings
{
public:
	static ResolutionSettings& Get();
	~ResolutionSettings();

	int AddResolution(glm::ivec2 _res);

	const glm::ivec2& Resolution(int _level);

	bool IsSet();

	void ModifyResolution(int _index, glm::ivec2 _res);

private:
	ResolutionSettings();

	std::vector<glm::ivec2> m_resolutionLevels;

	bool m_resolutionsAreSet = 0;

	//Only for server waiting for client startup
	const int RESOLUTION_LEVELS = 5;
};

