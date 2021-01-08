#include "frame.h"
#include <vector>

Frame::Frame(int ms, int del, Point3D pos, Point3D vel, Point3D spin, int pow, int timebonus, std::vector<MPState> mpstates, int gemcount, std::vector<int> gemstates, std::vector<int> ttstates, std::vector<int> powerupstates, std::string gamestate, std::vector<int> lmstates, int nextstatetime, std::vector<int> activepowstates, std::string gravityDir, std::vector<int> trapdoorDirs, std::vector<int> trapdoorOpen, std::vector<int> trapdoorClose, std::vector<float> trapdoorPos
#ifdef MBP
	,TeleportState tstate
	,CheckpointState cstate
	,bool eggstate
#endif
)
{
	this->ms = ms;
	this->deltaMs = del;
	this->position = pos;
	this->velocity = vel;
	this->spin = spin;
	this->powerup = pow;
	this->timebonus = timebonus;
	this->mpstates = mpstates;
	this->gemcount = gemcount;
	this->gemstates = gemstates;
	this->ttstates = ttstates;
	this->powerupstates = powerupstates;
	this->gamestate = gamestate;
	this->lmstates = lmstates;
	this->nextstatetime = nextstatetime;
	this->activepowstates = activepowstates;
	this->gravityDir = gravityDir;
	this->trapdoordirs = trapdoorDirs;
	this->trapdooropen = trapdoorOpen;
	this->trapdoorclose = trapdoorClose;
	this->trapdoorpos = trapdoorPos;
#ifdef MBP
	this->teleportState = tstate;
	this->checkpointState = cstate;
	this->eggstate = eggstate;
#endif
}

Frame::Frame()
{

}

Frame::~Frame()
{
}
