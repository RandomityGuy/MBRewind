#pragma once
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

void initiateLogging();

void logDebug(const char* str, va_list);

void logDebugV(const char* str, ...);

void stopLogging();