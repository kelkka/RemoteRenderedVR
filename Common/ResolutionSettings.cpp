#include "ResolutionSettings.h"

ResolutionSettings& ResolutionSettings::Get()
{
	static ResolutionSettings timers = ResolutionSettings();
	return timers;
}

ResolutionSettings::ResolutionSettings()
{
}


ResolutionSettings::~ResolutionSettings()
{
}

int ResolutionSettings::AddResolution(glm::ivec2 _res)
{
	m_resolutionLevels.push_back(_res);

	return int(m_resolutionLevels.size()) - 1;
}

const glm::ivec2& ResolutionSettings::Resolution(int _level)
{
	if (m_resolutionLevels.size() <= _level)
	{
		static glm::ivec2 def = glm::ivec2(0, 0);
		return def;
	}

	return m_resolutionLevels[_level];
}

bool ResolutionSettings::IsSet()
{
	if (m_resolutionsAreSet)
		return true;

	if (m_resolutionLevels.size() != RESOLUTION_LEVELS)
	{
		//AddResolution(glm::ivec2(1520, 1680));
		//AddResolution(glm::ivec2(1280, 720));
		//AddResolution(glm::ivec2(1280, 720));
		//AddResolution(glm::ivec2(1280, 720));
		//AddResolution(glm::ivec2(1280, 720));

		return false;
	}

	m_resolutionsAreSet = true;
	return true;
}

void ResolutionSettings::ModifyResolution(int _index, glm::ivec2 _res)
{
	m_resolutionLevels[_index] = _res;
}
