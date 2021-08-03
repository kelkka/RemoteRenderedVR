#include "CrashDump.h"
#include <string>
#include <time.h>
#include <fileapi.h>
#include <dbghelp.h>

void Crash(EXCEPTION_POINTERS* pep)
{
	// Open the file 

	std::wstring path;

	path += L"Crash_";

	time_t t = time(0);
	tm now;
	localtime_s(&now, &t);
	path += std::to_wstring((now.tm_year + 1900));
	path += L"-";
	path += std::to_wstring((now.tm_mon + 1));
	path += L"-";
	path += std::to_wstring(now.tm_mday);
	path += L" ";
	path += std::to_wstring(now.tm_hour);
	path += L"-";
	path += std::to_wstring(now.tm_min);
	path += L"-";
	path += std::to_wstring(now.tm_sec);
	path += L".dmp";

	HANDLE hFile = CreateFile(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	int answer = 0;

	if ((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE))
	{
		// Create the minidump 

		MINIDUMP_EXCEPTION_INFORMATION mdei;

		mdei.ThreadId = GetCurrentThreadId();
		mdei.ExceptionPointers = pep;
		mdei.ClientPointers = FALSE;

		MINIDUMP_TYPE mdt = (MINIDUMP_TYPE)(
			// 			MiniDumpWithFullMemory |
			// 			MiniDumpWithFullMemoryInfo |
			MiniDumpWithHandleData |
			MiniDumpWithThreadInfo);

		BOOL rv = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, mdt, (pep != 0) ? &mdei : 0, 0, 0);

		MessageBox(NULL, L"Program encountered an unknown error\nSee dump file", L"Fatal error", MB_OKCANCEL | MB_ICONINFORMATION);

		if (!rv)
			printf("MiniDumpWriteDump failed. Error: %u \n", GetLastError());
		else
			printf("Minidump created.\n");

		CloseHandle(hFile);
	}
	else
	{
		printf("CreateFile failed. Error: %u \n", GetLastError());
	}

	exit(-1);
}