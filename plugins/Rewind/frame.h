#pragma once
#include <TorqueLib/TGE.h>
#include <iostream>
#include <vector>
#include "RewindApi.h"

struct MPState
{
	float pathPosition;
	float targetPosition;
	TGE::PathedInterior* pathedInterior;
};

#ifdef MBP
struct TeleportState
{
	int teleportDelay;
	std::string destination;
	int teleportCounter;
};

struct CheckpointState
{
	std::string Obj;
	int gemCount;
	std::string gemStates;
	int powerup;
	std::string gravity;
	int respawnCounter;
	std::string respawnOffset;
};

#endif
class Frame
{
public:
	Frame();
	Frame(int ms, int del, Point3D pos, Point3D vel, Point3D spin, int pow, int timebonus, std::vector<MPState> mpstates, int gemcount, std::vector<int> gemstates, std::vector<int> ttstates, std::vector<int> powerupstates, std::string gamestate, std::vector<int> lmstates, int nextstatetime, std::vector<int> activepowstates, std::string gravityDir, std::vector<int> trapdoorDirs, std::vector<int> trapdoorOpen, std::vector<int> trapdoorClose, std::vector<float> trapdoorPos
#ifdef MBP
	,TeleportState teleportState //Lol wtf is this preprocessor hackery
	,CheckpointState checkpointState
	,bool eggstate
#endif
	);
	~Frame();
	int elapsedTime;
	int ms;
	int deltaMs;
	Point3D position;
	Point3D velocity;
	Point3D spin;
	int powerup;
	int timebonus;
	std::vector<MPState> mpstates;
    int gemcount;
	std::vector<int> gemstates;
	std::vector<int> ttstates;
	std::vector<int> powerupstates;
	std::string gamestate;
	std::vector<int> lmstates;
	int nextstatetime;
	std::vector<int> activepowstates;
	std::string gravityDir;
	std::vector<int> trapdoordirs;
	std::vector<int> trapdooropen;
	std::vector<int> trapdoorclose;
	std::vector<float> trapdoorpos;
#ifdef MBP
	TeleportState teleportState;
	CheckpointState checkpointState;
	bool eggstate;
#endif
	//std::vector<RewindableState> rewindableStates; // API STATES
	//std::vector<RewindableState> rewindableSOStates;

	// huge ass waste of memory

	std::vector<RewindableState<int>> rewindableIntStates;
	std::vector<RewindableState<float>> rewindableFloatStates;
	std::vector<RewindableState<bool>> rewindableBoolStates;
	std::vector<RewindableState<std::string>> rewindableStringStates;

	std::vector<RewindableState<int>> rewindableSOIntStates;
	std::vector<RewindableState<float>> rewindableSOFloatStates;
	std::vector<RewindableState<bool>> rewindableSOBoolStates;
	std::vector<RewindableState<std::string>> rewindableSOStringStates;
};

