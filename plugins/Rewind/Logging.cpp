#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <TorqueLib/TGE.h>
#include <TorqueLib/QuickOverride.h>
#include "Logging.h"
#ifdef __APPLE__
#include <unistd.h>
#else
#include <direct.h>
#endif

FILE* f;

void initiateLogging()
{
	std::string path = std::string(getcwd(NULL, 0));
#ifdef __APPLE__
	path += std::string("/rewind.log");
#else
	path += std::string("\\rewind.log");
#endif
	f = fopen(path.c_str(),"w");

	TGE::Con::printf("Opened %s for logging purposes", path.c_str());
}

void logDebug(const char* str, va_list args)
{
	if (f != NULL)
	{
		vfprintf(f, str, args);
		fprintf(f, "\n");
	}
}

void logDebugV(const char* str, ...)
{
	if (f != NULL)
	{
		va_list argptr;
		va_start(argptr, str);
		vfprintf(f, str, argptr);
		fprintf(f, "\n");
		va_end(argptr);
	}

}

void stopLogging()
{
	if (f != NULL)
	{
		fflush(f);
		fclose(f);
	}
}