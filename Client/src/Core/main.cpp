#include "MainEngine.h"
#include <DbgHelp.h>

void CreateMiniDump(EXCEPTION_POINTERS* pep)
{
	// Open the file 
	typedef BOOL(*PDUMPFN)(
		HANDLE hProcess,
		DWORD ProcessId,
		HANDLE hFile,
		MINIDUMP_TYPE DumpType,
		PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
		PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
		PMINIDUMP_CALLBACK_INFORMATION CallbackParam
		);

	time_t t = time(0);
	tm now;
	localtime_s(&now, &t);

	std::wstring archiveName = L"./";
	archiveName += std::to_wstring((now.tm_year + 1900)) + L"-";
	archiveName += std::to_wstring((now.tm_mon + 1)) + L"-";
	archiveName += std::to_wstring(now.tm_mday) + L"-";
	archiveName += std::to_wstring(now.tm_hour) + L"-";
	archiveName += std::to_wstring(now.tm_min) + L"-";
	archiveName += std::to_wstring(now.tm_sec);
	archiveName += L".dmp";

	HANDLE hFile = CreateFile(archiveName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	HMODULE h = ::LoadLibrary(L"DbgHelp.dll");
	PDUMPFN pFn = (PDUMPFN)GetProcAddress(h, "MiniDumpWriteDump");

	if ((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE))
	{
		// Create the minidump 

		MINIDUMP_EXCEPTION_INFORMATION mdei;

		mdei.ThreadId = GetCurrentThreadId();
		mdei.ExceptionPointers = pep;
		mdei.ClientPointers = TRUE;

		MINIDUMP_TYPE mdt = (MINIDUMP_TYPE)(
			MiniDumpWithHandleData |
			MiniDumpWithThreadInfo);

		BOOL rv = (*pFn)(GetCurrentProcess(), GetCurrentProcessId(), hFile, mdt, (pep != 0) ? &mdei : 0, 0, 0);

		// Close the file 
		CloseHandle(hFile);
	}
	else
	{
		printf("Minidump filehandle is null! %p\n", hFile);
	}
}

LONG WINAPI MyUnhandledExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
	CreateMiniDump(ExceptionInfo);
	return EXCEPTION_EXECUTE_HANDLER;
}

int main(int argc, char* args[])
{
	printf("Copyright (c) 2020, Viktor Kelkkanen <viktor.kelkkanen[]bth se>\n\n");

	SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);

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