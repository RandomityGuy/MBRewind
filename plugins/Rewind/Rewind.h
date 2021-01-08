#include "RewindManager.h"
#include <map>
#include <vector>

extern RewindManager rewindManager;
extern RewindManager ghostReplayManager;
#ifdef _DEBUG
extern std::vector<TGE::PathedInterior*> clientPathedInteriors; //Assuming nothing goes wrong we should have all the client PathedInteriors loaded in the mission
extern std::vector<TGE::PathedInterior*> serverPathedInteriors;
extern std::vector<TGE::ShapeBase*> clientTrapdoors;
#endif

extern std::map<int, TGE::PathedInterior*> serverToClientPIMap;
extern std::map<int, TGE::ShapeBase*> serverToClientSBMap;

extern Frame previousGhostFrame;
extern Frame previousFrame;

extern bool physicsOn;

extern bool lockTransforms;
extern U32 timeDelta;

struct MissionState
{
	std::vector<MPState> mpstates;
	std::vector<int> gemstates;
	std::vector<int> ttstates;
	std::vector<int> powerupstates;
	std::vector<int> explosivestates;

	std::vector<int> trapdoordirs;
	std::vector<int> trapdooropen;
	std::vector<int> trapdoorclose;
	std::vector<float> trapdoorpos;

#ifdef MBP
	bool eggstate;
#endif

	std::vector<RewindableState<int>> rewindableIntStates;
	std::vector<RewindableState<float>> rewindableFloatStates;
	std::vector<RewindableState<bool>> rewindableBoolStates;
	std::vector<RewindableState<std::string>> rewindableStringStates;
};

float getThreadPos(const char* staticshape);
float getThreadPos(int staticshape);
void setPathPosition(SimObjectId pathedInterior, int pos);

std::vector<int> GetPowerupTimeStates();
void SetPowerupTimeStates(std::vector<int> PowerupStates);
void SetTrapdoorThreadPos(SimObjectId id, float pos, bool isclient = false);
void GetMissionState(TGE::SimGroup* group, MissionState* missionstate);
void SetMissionState(TGE::SimGroup* group, MissionState* missionstate, MissionState* prevstate);
void RewindGhost(Frame* f);
void RewindFrame(Frame* f);
void StoreCurrentFrame(int ms);
void CallOnRewindEvent();
void CallOnRewindEventForSceneObjectBinding(TGE::SimGroup* group);