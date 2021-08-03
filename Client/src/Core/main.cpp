#include "MainEngine.h"

int main(int argc, char* args[])
{
	printf("Copyright (c) 2020, Viktor Kelkkanen <viktor.kelkkanen[]bth se>\n\n");

	{
		const char* configPath = "../content/netConfig_Client.txt";
		std::ifstream configFile;

		configFile.open(configPath, std::ifstream::in);

		if (configFile.good())
		{
			while (!configFile.eof())
			{
				std::string identifier;
				std::string value;

				configFile >> identifier;
				configFile >> value;

				if (identifier == "double")
				{
					//Contains 3 entries
					std::string paramVal;
					configFile >> paramVal;

					double dVal = std::stof(paramVal);

					StatCalc::SetParameter(value.c_str(), dVal);
				}
				else if (identifier == "string")
				{
					//Contains 3 entries
					std::string paramVal;
					configFile >> paramVal;

					StatCalc::SetParameterStr(value.c_str(), paramVal.c_str());
				}
			}

			configFile.close();
		}
		else
		{
			printf("Failed to read config: %s\n", configPath);
			system("pause");
		}
	}

	MainEngine* engine = new MainEngine();

	if (!engine->Init())
	{
		engine->Shutdown();
		return 1;
	}

	engine->GameLoop();

	engine->Shutdown();

	return 0;
}