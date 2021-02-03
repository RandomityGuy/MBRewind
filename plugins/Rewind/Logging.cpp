#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <filesystem>
#include <TorqueLib/TGE.h>
#include <TorqueLib/QuickOverride.h>
#include "Logging.h"

FILE* f;

void initiateLogging()
{
	std::filesystem::path path = std::filesystem::current_path();
	path /= "rewind.log";
	f = fopen(path.u8string().c_str(),"w");


	TGE::Con::printf("Opened %s for logging purposes", path.u8string().c_str());
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