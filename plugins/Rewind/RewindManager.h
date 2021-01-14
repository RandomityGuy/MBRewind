#pragma once
#include <vector>
#include "frame.h"
#include "RewindApi.h"
#include <thread>
#include "WorkerThread.h"
#include <assert.h>

template<typename... Args>
void DebugPrint(const char* printdata,Args... args)
{
	if (TGE::Con::getIntVariable("$Rewind::DebugInfo") == 1)
	{
		std::string out;
		extern int debugIndent;
		for (int i = 0; i < debugIndent; i++)
			out += std::string("  ");
		out += std::string(printdata);
		TGE::Con::printf(out.c_str(), args...);

		assert(debugIndent >= 0);
	}
}

template<typename... Args>
void DebugPush(const char* printdata, Args... args)
{
	extern int debugIndent;
	debugIndent++;
	assert(debugIndent >= 0);
	DebugPrint(printdata, args...);
}

template<typename... Args>
void DebugPop(const char* printdata, Args... args)
{
	extern int debugIndent;
	debugIndent--;
	assert(debugIndent >= 0);
	DebugPrint(printdata, args...);
}

struct ReplayInfo
{
	int version;
	int time;
	int elapsedTime;
	int frameCount;
	std::string replayPath;
	std::string replayMission;
};

class RewindManager
{
	std::vector<Frame> Frames;
	std::vector<std::vector<Frame>> SaveStates;
	std::mutex mutex;

public:
	std::string replayPath = std::string(".\\marble\\client\\replays\\testReplay.rwx");
	std::string replayMission = std::string("none");
	int totalTime = 0;
	int streamTimePosition = 0;
	float averageDelta = 0;
	int currentIndex;
	std::vector<TGE::PathedInterior*>* pathedInteriors;
	std::vector<RewindableBindingBase*> rewindableBindings;

#ifdef MBP
	std::string game = std::string("MBP");
#else
	std::string game = std::string("MBG");
#endif



public:
	RewindManager();
	RewindManager(const RewindManager&);
	~RewindManager();
	void pushFrame(Frame f);
	Frame popFrame(bool peek);
	Frame getFrameAt(int index);
	int getFrameCount();
	void save(std::string path);
	std::string load(std::string path,bool isGhost = false);
	ReplayInfo analyze(std::string path);
	void clear(bool write, Worker* worker);
	Frame interpolateFrame(Frame one, Frame two, float ratio, float delta);
	template<typename T>
	RewindableState<T> InterpolateRewindableState(RewindableState<T> one, RewindableState<T> two, float ratio, float delta);
	Frame* getFrameAtMs(float ms,int index,bool useElapsed = true);
	Frame* getFrameAtElapsedMs(float ms);
	Frame* getNextFrame(float delta);
	Frame* getNextRewindFrame(float delta);
	Frame* getNextNonElapsedFrame(float delta);
	Frame* getRealtimeFrameAtMs(float ms);
	int getSavedStateCount();
	void saveState();
	void loadState(int saveStateIndex);
	void clearSaveStates();
	void spliceReplayFromMs(float ms);

	inline bool hasMs(int ms)
	{
		for (auto& it : Frames)
		{
			if (it.ms == ms)
				return true;
		}
		return false;
	}

	inline void setUpFrameStreaming()
	{
		currentIndex = Frames.size() - 1;
		streamTimePosition = 0;
		averageDelta = 0;
		for (auto& frame : Frames)
			averageDelta += frame.deltaMs;

		averageDelta /= Frames.size();
	}

	inline Frame getFrameAtIndex()
	{
		return Frames[currentIndex];
	}
	inline void setFrameElapsedTimes()
	{
		if (totalTime == 0)
		{
			for (int i = Frames.size() - 1; i >= 0; i--)
			{
				totalTime += Frames[i].deltaMs;
				Frames[i].elapsedTime = totalTime;
			}
		}
	}
};

