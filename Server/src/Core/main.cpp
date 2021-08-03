#include "Engine.h"
#include "MainEngineGL.h"
#include "MainEngineD3D.h"


int main(int argc, char* args[])
{
	printf("Copyright (c) 2020, Viktor Kelkkanen <viktor.kelkkanen[]bth se>\n\n");

	{
		const char* configPath = "../content/netConfig_Server.txt";
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

	{
		Engine* program = nullptr;

		if (StatCalc::GetParameter("USE_D3D11") > 0)
		{
			program = new MainEngineD3D(argc, args);
		}
		else
		{
			program = new MainEngineGL(argc, args);
		}

		if (!program->Init())
		{
			program->Shutdown();
			return 1;
		}

		program->GameLoop();

		program->Shutdown();

		delete program;
	}



	return 0;
}