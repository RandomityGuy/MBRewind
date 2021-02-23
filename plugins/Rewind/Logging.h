#pragma once
#include <TorqueLib/TGE.h>

void initiateLogging();

void logDebug(const char* str, va_list);

void logDebugV(const char* str, ...);

void stopLogging();

template<typename... Args>
void DebugPrint(const char* printdata, Args... args)
{
	if (TGE::Con::getIntVariable("$Rewind::DebugInfo") == 1)
	{
		std::string out;
		extern int debugIndent;
		for (int i = 0; i < debugIndent; i++)
			out += std::string("  ");
		out += std::string(printdata);

		logDebugV(out.c_str(), args...);
		//TGE::Con::printf(out.c_str(), args...);

		assert(debugIndent >= 0);
	}
}

template<typename... Args>
void DebugPush(const char* printdata, Args... args)
{
	extern int debugIndent;
	debugIndent++;
	if (debugIndent < 0)
	{
		DebugPrint("Bad indent level, resetting to 0");
		debugIndent = 0;
	}
	DebugPrint(printdata, args...);
}

template<typename... Args>
void DebugPop(const char* printdata, Args... args)
{
	extern int debugIndent;
	debugIndent--;
	if (debugIndent < 0)
	{
		DebugPrint("Bad indent level, resetting to 0");
		debugIndent = 0;
	}
	DebugPrint(printdata, args...);
}

template<typename T>
inline void deleteSafe(T* obj)
{
	DebugPrint("Attempting delete %s", typeid(T).name());
	if (obj != NULL)
	{
		DebugPrint("Success");
		delete obj;
	}
	else
		DebugPrint("Failed");
}