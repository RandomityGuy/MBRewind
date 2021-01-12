// Rewind.cpp : Defines the exported functions for the DLL application.
//
#include <cstdio>
#include <TorqueLib/math/mMath.h>
#include <TorqueLib/math/mathUtils.h>
#include <TorqueLib/TGE.h>
#include <TorqueLib/QuickOverride.h>
#include <PluginLoader/PluginInterface.h>
#include "StringMath.h"
#include <vector>
#include <stdio.h>
#include "Rewind.h"
#include "RewindApi.h"
#include "WorkerThread.h"
#ifdef __APPLE__
#include <sys/stat.h>
#include <unistd.h>
#endif


RewindManager rewindManager;
RewindManager ghostReplayManager;
#ifdef _DEBUG
std::vector<TGE::PathedInterior*> clientPathedInteriors; //Assuming nothing goes wrong we should have all the client PathedInteriors loaded in the mission
std::vector<TGE::PathedInterior*> serverPathedInteriors;
std::vector<TGE::ShapeBase*> clientTrapdoors;
#endif

std::map<int, TGE::PathedInterior*> serverToClientPIMap;
std::map<int, TGE::ShapeBase*> serverToClientSBMap;
std::vector<TGE::TSShapeInstance*> trapdoorShapeInstances;

Frame previousGhostFrame;
Frame previousFrame;

Worker workerThread;

bool physicsOn = true;

bool lockTransforms = false;
U32 timeDelta = 0;

F32 replayTimeDelta = 0;

int debugIndent = 0;

std::vector<std::string> executeQueue;
std::mutex executeMutex;

//---------------------------------------------------------------------------------------
// Hacky Async

ConsoleFunction(tickAsync, void, 1, 1, "tickAsync()")
{
	executeMutex.lock();
	if (!executeQueue.empty())
	{
		for (auto& str : executeQueue) {
			TGE::Con::evaluatef(str.c_str());
		}
	}
	executeQueue.clear();
	executeMutex.unlock();
}


//---------------------------------------------------------------------------------------
// API Functions

ConsoleFunction(registerRewindable, void, 4, 4, "registerRewindable(string namespace,int type,int storagetype)")
{
	int storagetype = atoi(argv[3]);
	
	DebugPush("Entering registerRewindable(%s,%s,%s)", argv[1], argv[2], argv[3]);

	if (storagetype == 0)
	{
		RewindableBinding<int> binding = RewindableBinding<int>((RewindableType)atoi(argv[2]), std::string(argv[1]));
		rewindManager.rewindableBindings.push_back(new RewindableBinding<int>(binding));
		ghostReplayManager.rewindableBindings.push_back(new RewindableBinding<int>(binding));
	}
	else if (storagetype == 1)
	{
		RewindableBinding<float> binding = RewindableBinding<float>((RewindableType)atoi(argv[2]), std::string(argv[1]));
		rewindManager.rewindableBindings.push_back(new RewindableBinding<float>(binding));
		ghostReplayManager.rewindableBindings.push_back(new RewindableBinding<float>(binding));
	}
	else if (storagetype == 2)
	{
		RewindableBinding<bool> binding = RewindableBinding<bool>((RewindableType)atoi(argv[2]), std::string(argv[1]));
		rewindManager.rewindableBindings.push_back(new RewindableBinding<bool>(binding));
		ghostReplayManager.rewindableBindings.push_back(new RewindableBinding<bool>(binding));
	}
	else if (storagetype == 3)
	{
		RewindableBinding<std::string> binding = RewindableBinding<std::string>((RewindableType)atoi(argv[2]), std::string(argv[1]));
		rewindManager.rewindableBindings.push_back(new RewindableBinding<std::string>(binding));
		ghostReplayManager.rewindableBindings.push_back(new RewindableBinding<std::string>(binding));
	}
	DebugPop("Leaving registerRewindable()");
}

ConsoleFunction(unregisterRewindable, void, 2, 2, "unregisterRewindable(string namespace)")
{
	DebugPush("Entering unregisterRewindable(%s)", argv[1]);

	for (size_t i = 0; i < rewindManager.rewindableBindings.size(); i++)
	{
		if (strcmp(rewindManager.rewindableBindings[i]->BindingNamespace.c_str(), argv[1]) == 0)
		{
			rewindManager.rewindableBindings.erase(rewindManager.rewindableBindings.begin() + i);

			for (size_t i = 0; i < ghostReplayManager.rewindableBindings.size(); i++)
			{
				if (strcmp(ghostReplayManager.rewindableBindings[i]->BindingNamespace.c_str(), argv[1]) == 0)
				{
					ghostReplayManager.rewindableBindings.erase(ghostReplayManager.rewindableBindings.begin() + i);
					return;
				}
			}

		}
	}

	DebugPush("Entering unregisterRewindable()");
}

ConsoleFunction(callOnRewindEvent, void, 1, 1, "callOnRewindEvent()")
{
	CallOnRewindEvent();
}

ConsoleFunction(setGame, void, 2, 2, "setGame(string game)")
{
	DebugPush("Entering setGame(%s)", argv[1]);
	rewindManager.game = std::string(argv[1]);
	ghostReplayManager.game = std::string(argv[1]);
	DebugPop("Leaving setGame()");
}

//---------------------------------------------------------------------------------------
// Utility Functions
ConsoleFunction(fileExists, bool, 2, 2, "fileExists(string path)") //Because TGE sucks so much that it doesnt detect new files after it has started
{
	struct stat b;
	return (stat(argv[1], &b) == 0);
}

ConsoleFunction(getWordDelim, const char*, 4, 4, "getWordDelim(str,delim,index)")
{
	char* token = strtok((char*)argv[1], argv[2]);
	if (token == NULL) return "0";
	int tokenindex = 0;
	if (atoi(argv[3]) == 0) return token;
	while (token != NULL)
	{
		tokenindex++;
		token = strtok(NULL, argv[2]);
		if (tokenindex == atoi(argv[3])) return token;
	}
	return "0";

}

ConsoleFunction(getWordCountDelim, int, 3, 3, "getWordCountDelim(str,delim)")
{
	char* token = strtok((char*)argv[1], argv[2]);
	if (token == NULL) return 0;
	int tokenindex = 0;
	while (token != NULL)
	{
		tokenindex++;
		token = strtok(NULL, argv[2]);
	}
	return tokenindex;
}

ConsoleFunction(combineAngAxis, const char*, 3, 3, "combineAngAxis(AngAxisF lhs,AngAxisF rhs)")
{
	Point3F a1, a2;
	float ang1, ang2;
	sscanf(argv[1], "%f %f %f %f", &a1.x, &a1.y, &a1.z, &ang1);
	sscanf(argv[2], "%f %f %f %f", &a2.x, &a2.y, &a2.z, &ang2);

	AngAxisF ret = AngAxisF(QuatF().mul(QuatF(a1, ang1), QuatF(a2, ang2)));
	char* buf = TGE::Con::getReturnBuffer(256);
	sprintf(buf, "%f %f %f %f", ret.axis.x, ret.axis.y, ret.axis.z, ret.angle);
	return buf;
}

//---------------------------------------------------------------------------------------
// Rewind / Replay Functions
ConsoleFunction(setReplayPath, void, 2, 2, "setReplayPath(string path)")
{
	DebugPush("Entering setReplayPath");
	rewindManager.replayPath = std::string(argv[1]);
	DebugPop("Leaving setReplayPath");
}

ConsoleFunction(setReplayMission, void, 2, 2, "setReplayMission(string path)")
{
	DebugPush("Entering setReplayMission");
	rewindManager.replayMission = std::string(argv[1]);
	// Terrible place for this to be in, but this function is just called once per mission so /shrug
	rewindManager.clearSaveStates();
	DebugPop("Leaving setReplayMission");
}

ConsoleFunction(getReplayPath, const char *, 1, 1, "getReplayPath()")
{
	DebugPush("Entering getReplayPath");
	char* retbuffer = TGE::Con::getReturnBuffer(1024);
	strcpy(retbuffer, rewindManager.replayPath.c_str());
	DebugPop("Leaving getReplayPath");
	return retbuffer;
}

ConsoleFunction(loadReplay,const char *, 2, 3, "loadReplay(string path,bool ghostreplay = false)")
{
	TGE::Con::printf("Loading replay %s", argv[1]);
	char* retbuff = TGE::Con::getReturnBuffer(1024);
	if (argc > 2)
	{
		if (atoi(argv[2]) == 1)
			strcpy(retbuff, ghostReplayManager.load(std::string(argv[1]),true).c_str());
	}
	else
		strcpy(retbuff, rewindManager.load(std::string(argv[1])).c_str());
	return retbuff;
}

ConsoleFunction(getAverageFrameDelta, F32, 1, 1, "getAverageFrameDelta()")
{
	F32 avg = 0;
	for (int i = 0; i < rewindManager.getFrameCount(); i++)
		avg += rewindManager.getFrameAt(i).deltaMs;

	return avg / rewindManager.getFrameCount();
}

ConsoleFunction(clearFrames, void, 1, 2, "clearFrames(bool write)")
{
	rewindManager.clear(atoi(argv[1]), &workerThread);
}

ConsoleFunction(rewindFrame_internal, bool, 1, 2, "rewindFrame_internal(delta)")
{
	Frame* f = NULL;
	if (argc == 2)
	{
		if (rewindManager.getFrameCount() == 0) f = NULL;
		if (rewindManager.streamTimePosition + (atof(argv[1])) < 0) f = NULL;
		f = rewindManager.getNextFrame(atof(argv[1]));
		replayTimeDelta = atof(argv[1]);
	}

	if (f == NULL && argc == 2)
		return false;

	RewindFrame(f);
	return true;
		
}

ConsoleFunction(rewindToMs, bool, 2, 2, "rewindToMs(ms)")
{
	Frame* f = NULL;
	if (rewindManager.getFrameCount() == 0)
		f = NULL;
	f = rewindManager.getFrameAtElapsedMs(atof(argv[1]));

	if (f == NULL)
		return false;

	RewindFrame(f);
	return true;
}

ConsoleFunction(rewindGhost_internal, void, 2, 2, "rewindGhost_internal(delta)")
{
	RewindGhost(ghostReplayManager.getRealtimeFrameAtMs(atof(argv[1])));
}

ConsoleFunction(storeFrame, void, 2, 2, "storeFrame(ms)")
{
	StoreCurrentFrame(atoi(argv[1]));
}

ConsoleFunction(getFrameCount, int, 1, 1, "getFrameCount()")
{
	return rewindManager.getFrameCount();
}

ConsoleFunction(saveState, void, 1, 1, "saveState()")
{
	rewindManager.saveState();
}

ConsoleFunction(loadState, void, 2, 2, "loadState(int state)")
{
	rewindManager.loadState(StringMath::scan<int>(argv[1]));
}

ConsoleFunction(getStateCount, int, 1, 1, "getStateCount()")
{
	return rewindManager.getSavedStateCount();
}

ConsoleFunction(analyzeReplay, void, 3, 3, "analyzeReplay(string path, function onReplayLoaded(scriptObject))")
{
	std::string path = std::string(argv[1]);
	std::string callback = std::string(argv[2]);

	workerThread.addTask([=]() {
		TGE::Con::printf("Analyzing replay %s", path.c_str());
		char buf[1024];

		ReplayInfo info = rewindManager.analyze(path);

		sprintf(buf, "$ReplayAnalysisReturn = new ScriptObject(ReplayAnalysis) { version = %d; framecount = %d; time = %d; elapsedtime = %d; replaymission = \"%s\"; };", info.version, info.frameCount, info.time, info.elapsedTime, info.replayMission.c_str());

		executeMutex.lock();
		executeQueue.push_back(std::string(buf));

		char buf2[1024];
		sprintf(buf2, "%s($ReplayAnalysisReturn);", callback.c_str());
		executeQueue.push_back(std::string(buf2));
		executeMutex.unlock();
	});
}

ConsoleFunction(spliceReplay, void, 2, 2, "spliceReplay(float ms)")
{
	rewindManager.spliceReplayFromMs(atof(argv[1]));
}

//---------------------------------------------------------------------------------------
// Extra Marble Physics Functions
//It got too late when I figured that I could use ConsoleMethod instead for these functions and im too lazy to replace em.

ConsoleMethod(ShapeBase, getCameraTransform, const char*, 3, 3, "getCameraTransform(pos)")
{
	MatrixF mat;
	float pos = atof(argv[2]);
	object->getCameraTransform(&pos, &mat);
	return StringMath::print(mat);
}

ConsoleFunction(getVelocity, const char *, 2, 2, "getVelocity(Marble m)")
{
	TGE::Marble *marble = static_cast<TGE::Marble*>(TGE::Sim::findObject(argv[1]));
	Point3D vel = marble->getVelocity();
	return StringMath::print(vel);	
}

ConsoleFunction(setVelocity, void, 3, 3, "setVelocity(Marble m,Point3D vel)")
{
	TGE::Marble *marble = static_cast<TGE::Marble*>(TGE::Sim::findObject(argv[1]));
	marble->setVelocity(StringMath::scan<Point3D>(argv[2]));
}

ConsoleFunction(getAngularVelocity, const char *, 2, 2, "getAngularVelocity(Marble m)")
{
	TGE::Marble *marble = static_cast<TGE::Marble*>(TGE::Sim::findObject(argv[1]));
	Point3D vel = marble->getAngularVelocity();
	return StringMath::print(vel);
}

ConsoleFunction(setAngularVelocity, void, 3, 3, "setAngularVelocity(Marble m,Point3D vel)")
{
	TGE::Marble *marble = static_cast<TGE::Marble*>(TGE::Sim::findObject(argv[1]));
	marble->setAngularVelocity(StringMath::scan<Point3D>(argv[2]));
}

ConsoleFunction(setPosition, void, 3, 3, "setPosition(Marble m,Point3D pos)")
{
	TGE::Marble *marble = static_cast<TGE::Marble*>(TGE::Sim::findObject(argv[1]));
	marble->setPositionSimple(StringMath::scan<Point3D>(argv[2]));
}

ConsoleFunction(setPhysics, void, 2, 2, "setPhysics(bool on)")
{
	physicsOn = atoi(argv[1]);
}

ConsoleFunction(lockTransforms, void, 2, 2, "lockTransforms(bool lock)")
{
	lockTransforms = atoi(argv[1]);
}

TorqueOverrideMember(void, Marble::advancePhysics, (TGE::Marble* thisObj, const TGE::Move* move, U32 delta), origAdvPhysics)
{

	MatrixF transform;
	Point3D vel, spin;
	if (lockTransforms)
	{
		transform = thisObj->getTransform();
		vel = thisObj->getVelocity();
		spin = thisObj->getAngularVelocity();
	}
	if (physicsOn)
		origAdvPhysics(thisObj, move, delta);
	if (lockTransforms)
	{
		thisObj->setTransform(transform);
		thisObj->setVelocity(Point3D(0, 0, 0));
		thisObj->setAngularVelocity(spin);
	}

};

//---------------------------------------------------------------------------------------
// Extra MP Functions
ConsoleFunction(getPathPosition,float, 2, 2, "getPathPosition(PathedInterior pathedInterior)")
{
	TGE::PathedInterior *p = static_cast<TGE::PathedInterior*>(TGE::Sim::findObject(argv[1]));

	std::map<int, TGE::PathedInterior*>::iterator it = serverToClientPIMap.find(p->getId());
	if (it == serverToClientPIMap.end())
		return 0;
	else
		return serverToClientPIMap.at(p->getId())->getPathPosition(); //The path position stored in client PathedInterior is more precise, so we just give this, this fixes the MP jitter bug

}

ConsoleFunction(setTargetPosition, void, 3, 3, "setTargetPosition(PathedInterior pathedInterior,int target)")
{
	TGE::PathedInterior* pServerd = static_cast<TGE::PathedInterior*>(TGE::Sim::findObject(argv[1]));

	TGE::PathedInterior* p = NULL;

	std::map<int, TGE::PathedInterior*>::iterator it = serverToClientPIMap.find(pServerd->getId());
	if (it == serverToClientPIMap.end())
		p = pServerd;
	else
		p = serverToClientPIMap.at(pServerd->getId());

	double pathpos = atof(argv[2]);

	//ResolvePathKey():
	if (p->getPathKey() == 0xFFFFFFFF || !p->isClientObject())
	{
		p->setPathKey(p->getPathKey2());
		Point3F pathPos;
		Point3F initialPos;
		p->getBaseTransform().getColumn(3, &initialPos);
		TGE::gServerPathManager->getPathPosition(p->getPathKey(), 0, pathPos);
		p->setOffset(initialPos - pathPos);
	}

	if (pathpos < -2)
		pathpos = 0;
	if (pathpos > TGE::gServerPathManager->getPathTotalTime(p->getPathKey()))
		pathpos = TGE::gServerPathManager->getPathTotalTime(p->getPathKey());

	if (p->getTargetPosition() != pathpos)
	{
		pServerd->setTargetPosition(pathpos);
		p->setTargetPosition(pathpos);
		p->setMaskBits(0x8);
	}
}

ConsoleFunction(setPathPosition, void, 3, 3, "setPathPositionF32(PathedInterior pathedInterior,double path)")
{
	setPathPosition(atoi(argv[1]), atoi(argv[2]));
}

ConsoleFunction(getTargetPosition, int, 2, 2, "getTargetPosition(PathedInterior pathedInterior)")
{
	TGE::PathedInterior* p = static_cast<TGE::PathedInterior*>(TGE::Sim::findObject(argv[1]));
	return p->getTargetPosition();
}

TorqueOverrideMember(void, PathedInterior::computeNextPathStep, (TGE::PathedInterior* thisObj, U32 delta), origCNP)
{
	int curtarget = thisObj->getTargetPosition();
	double curpath = thisObj->getPathPosition();
	if (physicsOn)
		origCNP(thisObj, delta);
	if (lockTransforms)
	{
		thisObj->setTargetPosition(curtarget);
		thisObj->setPathPosition(curpath);
	}
}

TorqueOverrideMember(void, PathedInterior::advance, (TGE::PathedInterior* thisObj, double delta), origAdv)
{
	timeDelta = delta;
	int curtarget = thisObj->getTargetPosition();
	double curpath = thisObj->getPathPosition();
	if (physicsOn && !lockTransforms)
		origAdv(thisObj, delta);
	if (lockTransforms)
	{
		thisObj->setTargetPosition(curtarget);
		thisObj->setPathPosition(curpath);
	}
}

void PI_ProcessTick(TGE::PathedInterior* PI, const TGE::Move* move);

TorqueOverrideMember(void, PathedInterior::processTick, (TGE::PathedInterior* thisObj, const TGE::Move* move), origprocTick)
{
	//why the heck does torque even do processTick in 32ms intervals
	int curtarget = thisObj->getTargetPosition();
	double curpath = thisObj->getPathPosition();
	if (physicsOn && !lockTransforms)
	{
		PI_ProcessTick(thisObj, move);
	}
	if (lockTransforms)
	{
		thisObj->setTargetPosition(curtarget);
		thisObj->setPathPosition(curpath);
	}
}

void PI_ProcessTick(TGE::PathedInterior* PI, const TGE::Move* move)
{
	if (PI->isServerObject())
	{
		U32 timeMs = 32;
		if (PI->getPathPosition() != PI->getTargetPosition())
		{
			S32 delta;
			if (PI->getTargetPosition() < 0)
			{
				if (PI->getTargetPosition() == -1)
					delta = timeMs;
				else if (PI->getTargetPosition() == -2)
					delta = -timeMs;
				PI->setPathPosition(PI->getPathPosition() + delta);
				U32 totalTime = TGE::gClientPathManager->getPathTotalTime(PI->getPathKey2());
				while (PI->getPathPosition() >= totalTime)
					PI->setPathPosition(PI->getPathPosition() - totalTime);
				while (PI->getPathPosition() < 0)
					PI->setPathPosition(PI->getPathPosition() + totalTime);
			}
			else
			{
				delta = PI->getTargetPosition() - PI->getPathPosition();
				if (delta < -timeMs)
					delta = -timeMs;
				else if (delta > timeMs)
					delta = timeMs;
				PI->setPathPosition(PI->getPathPosition() + delta);
			}
		}
	}
}

TorqueOverrideMember(U32, PathedInterior::packUpdate, (TGE::PathedInterior* thisObj, TGE::NetConnection* con, U32 mask, TGE::BitStream* stream), origPI_PackUpdate)
{
	U32 ret = origPI_PackUpdate(thisObj, con, mask, stream);
	stream->writeInt(thisObj->getId(), 32);
	return ret;
}

TorqueOverrideMember(void, PathedInterior::unpackUpdate, (TGE::PathedInterior* thisObj, TGE::NetConnection* con, TGE::BitStream* stream), origPI_UnpackUpdate)
{
	origPI_UnpackUpdate(thisObj, con, stream);
	int serverObj = stream->readInt(32);

	auto it = serverToClientPIMap.find(serverObj);
	if (it == serverToClientPIMap.end())
	{
		serverToClientPIMap.insert(serverToClientPIMap.begin(), std::pair<int, TGE::PathedInterior*>(serverObj, thisObj));
	}
}

TorqueOverrideMember(void, PathedInterior::onRemove, (TGE::PathedInterior* thisObj), origOnRemove)
{
#ifdef _DEBUG
	if (thisObj->isClientObject())
	{
		std::vector<TGE::PathedInterior*>::iterator it = std::find(clientPathedInteriors.begin(), clientPathedInteriors.end(), thisObj);
		clientPathedInteriors.erase(it);
		TGE::Con::printf("Removing PathedInterior %s to ClientList", thisObj->getIdString());
	}
#endif
	if (thisObj->isServerObject())
	{
#ifdef _DEBUG
		std::vector<TGE::PathedInterior*>::iterator it = std::find(serverPathedInteriors.begin(), serverPathedInteriors.end(), thisObj);
		serverPathedInteriors.erase(it);
		TGE::Con::printf("Removing PathedInterior %s to ServerList", thisObj->getIdString());
#endif
		serverToClientPIMap.erase(thisObj->getId());
	}
	origOnRemove(thisObj);
}

#ifdef _DEBUG
ConsoleFunction(getPathPositionF32, float, 2, 2, "getPathPositionF32(PathedInterior pathedInterior)")
{
	TGE::PathedInterior* p = static_cast<TGE::PathedInterior*>(TGE::Sim::findObject(argv[1]));

	float path = p->getPathPositionF32();

	return path;
}
#endif

//---------------------------------------------------------------------------------------
// StaticShape Thread Control
ConsoleFunction(getThreadDir, F32, 2, 2, "getThreadDir(StaticShape s)")
{
	//return 0; 
	//This returns ShapeBase::mScriptThread[0].forward, found this in ShapeBase::advanceThreads with a bit of pointer math
	TGE::ShapeBase *s = static_cast<TGE::ShapeBase*>(TGE::Sim::findObject(argv[1]));
	return s->getThread1Forward(); 
}

ConsoleFunction(setThreadPos, void, 3, 3, "setThreadPos(StaticShape s,float pos)")
{
	//Stuff got found being referenced in ShapeBase::advanceThreads
	TGE::ShapeBase *s = static_cast<TGE::ShapeBase*>(TGE::Sim::findObject(argv[1]));

	TGE::TSThread *t = s->getThread1(); //This returns ShapeBase::mScriptThread[0].thread
	if (t != 0)
	{
		TGE::TSShapeInstance *ts = s->getTSShapeInstance(); 
		t->setPos(atof(argv[2])); //This is a setter function, the offset of pos is 0xC on windows, probably the same in mac
		ts->setPos(t, atof(argv[2])); //Now this looks like a setter function but it isnt, its the TSShapeInstance::setPos member function, also found being reference somewhere
		//Its a setter function that sets ShapeBase::mScriptThread[0].forward, 
		//TGE::Thread::State is same as the one in TGE except Thread::State::FromMiddle is a new one I added
		s->setThread1State(TGE::Thread::State::FromMiddle); 
	}
}

TorqueOverrideMember(void, TSShapeInstance::advanceTime, (TGE::TSShapeInstance* thisObj, F32 delta), origTSAdvanceTime)
{
	if (std::find(trapdoorShapeInstances.begin(), trapdoorShapeInstances.end(), thisObj) != trapdoorShapeInstances.end())
	{
		origTSAdvanceTime(thisObj, delta);
		return;
	}

	bool rewinding = TGE::Con::getIntVariable("$rewinding") == 1 ? true : false;
	if (rewinding)
		origTSAdvanceTime(thisObj,-delta);
	else
	{
		bool isReplay = TGE::Con::getIntVariable("$Rewind::IsReplay") == 1 ? true : false;
		if (isReplay)
		{
			origTSAdvanceTime(thisObj, replayTimeDelta / 1000);
		}
		else
			origTSAdvanceTime(thisObj, delta);
	}
}

const char* getField2(TGE::SimObject* obj, const char* field)
{
	const char* name = TGE::StringTable->insert(field, false);
	return (const_cast<TGE::SimObject*>(obj))->getDataField(name, NULL);
}

TorqueOverrideMember(void, Item::advanceTime, (TGE::Item* thisObj, F32 dt), origAdvanceTime)
{
	TGE::SimObject* PlayGui = TGE::Sim::findObject("PlayGui");
	int time = atoi(getField2(PlayGui, "totalTime"));
	int simtime = TGE::Sim::gCurrentTime;
	TGE::Sim::gCurrentTime = time;
	origAdvanceTime(thisObj, dt);
	TGE::Sim::gCurrentTime = simtime;
}

#ifdef _DEBUG
ConsoleFunction(getThreadState, int, 2, 2, "getThreadState(StaticShape s)")
{
	TGE::ShapeBase *s = static_cast<TGE::ShapeBase*>(TGE::Sim::findObject(argv[1]));
	return s->getThread1State();
}
#endif

ConsoleFunction(getThreadPos, F32, 2, 2, "getThreadPos(StaticShape s)")
{
	return getThreadPos(argv[1]);
}

#ifdef _DEBUG
ConsoleFunction(computePathStep, void, 3, 3, "computePathStep(int delta)")
{
	TGE::PathedInterior *p = static_cast<TGE::PathedInterior*>(TGE::Sim::findObject(argv[1]));
	p->computeNextPathStep(atoi(argv[2]));
}

ConsoleFunction(advanceMP, void, 3, 3, "advanceMP(int delta)")
{
	TGE::PathedInterior *p = static_cast<TGE::PathedInterior*>(TGE::Sim::findObject(argv[1]));
	p->advance(atoi(argv[2]));
}

ConsoleFunction(ListClientPathedInteriors,void, 1, 1, "ListClientPathedInteriors()")
{
	for (int i = 0; i < clientPathedInteriors.size(); i++)
	{
		TGE::Con::printf("PathedInterior %s", clientPathedInteriors.at(i)->getIdString());
	}
}

ConsoleFunction(ListClientServerMap, void, 1, 1, "ListClientPathedInteriors()")
{
	for (auto& it : serverToClientPIMap)
	{
		TGE::Con::printf("PI %d -> %d", it.first, it.second->getId());
	}
}

ConsoleFunction(ListServerPathedInteriors, void, 1, 1, "ListServerPathedInteriors()")
{
	for (int i = 0; i < serverPathedInteriors.size(); i++)
	{
		TGE::Con::printf("PathedInterior %s", serverPathedInteriors.at(i)->getIdString());
	}
}
#endif

#ifdef _DEBUG
TorqueOverrideMember(bool, PathedInterior::onAdd, (TGE::PathedInterior *thisObj), origOnAdd)
{
	if (thisObj->isClientObject())
	{
		clientPathedInteriors.push_back(thisObj);
		TGE::Con::evaluatef("%s.clientID=%s;", thisObj->getIdString(), thisObj->getIdString());
		TGE::Con::printf("Adding PathedInterior %s to ClientList", thisObj->getIdString());
	}
	if (thisObj->isServerObject())
	{
		serverPathedInteriors.push_back(thisObj);
		TGE::Con::printf("Adding PathedInterior %s to ServerList", thisObj->getIdString());
	}
	return origOnAdd(thisObj);
}
#endif

//---------------------------------------------------------------------------------------
// ShapeBase overrides

TorqueOverrideMember(U32, ShapeBase::packUpdate, (TGE::ShapeBase *thisObj, TGE::NetConnection *connection, U32 mask, TGE::BitStream *stream), origSB_packUpdate)
{
	U32 ret = origSB_packUpdate(thisObj, connection, mask, stream);
	stream->writeInt(thisObj->getId(), 32);
	return ret;
}

TorqueOverrideMember(void, ShapeBase::unpackUpdate, (TGE::ShapeBase *thisObj, TGE::NetConnection *con, TGE::BitStream *stream), origSB_unpackUpdate)
{
	origSB_unpackUpdate(thisObj, con, stream);
	int serverObj = stream->readInt(32);

	auto it = serverToClientSBMap.find(serverObj);
	if (it == serverToClientSBMap.end())
	{
		serverToClientSBMap.insert(serverToClientSBMap.begin(), std::pair<int, TGE::ShapeBase*>(serverObj, thisObj));
	}
}

//We need to make use of the new TGE::Thread::FromMiddle value
//For TGE::Thread, just copy its field members from TGE source
TorqueOverrideMember(void, ShapeBase::updateThread, (TGE::ShapeBase* thisObj, TGE::Thread& st), origUpdateThread)
{
	TGE::TSShapeInstance* mShapeInstance = thisObj->getTSShapeInstance();
	switch (st.state) {
	case TGE::Thread::Stop:
		mShapeInstance->setTimeScale(st.thread, 1); //This is a member function, TSShapeInstance::setTimeScale()
		mShapeInstance->setPos(st.thread, 0);
		// Drop through to pause state
	case TGE::Thread::Pause:
		mShapeInstance->setTimeScale(st.thread, 0);
		break;
	case TGE::Thread::Play:
		if (st.atEnd) {
			mShapeInstance->setTimeScale(st.thread, 1);
			mShapeInstance->setPos(st.thread, st.forward ? 1 : 0);
			mShapeInstance->setTimeScale(st.thread, 0);
		}
		else {
			mShapeInstance->setTimeScale(st.thread, st.forward ? 1 : -1);
		}
		break;

	case TGE::Thread::FromMiddle:
		mShapeInstance->setTimeScale(st.thread, 1);
		mShapeInstance->setPos(st.thread, st.thread->getPos());
		st.atEnd = false;
		st.state = TGE::Thread::Pause;
		break;

	}

}

#ifdef _DEBUG
TorqueOverrideMember(bool, ShapeBase::onAdd, (TGE::ShapeBase *thisObj), origOnAddShapeBase)
{
	if (thisObj->isClientObject())
	{
		if (strcmp(thisObj->getDataBlock()->getName(), "TrapDoor")==0)
		{
			clientTrapdoors.push_back(thisObj);
			trapdoorShapeInstances.push_back(thisObj->getTSShapeInstance());
			TGE::Con::evaluatef("%s.clientID=%s;", thisObj->getIdString(), thisObj->getIdString());
			TGE::Con::printf("Adding Shape %s(%s) to ClientList", thisObj->getIdString(), thisObj->getDataBlock()->getName());
		}
	}
	return origOnAddShapeBase(thisObj);
}
#endif

TorqueOverrideMember(void, ShapeBase::onRemove, (TGE::ShapeBase *thisObj), origOnRemoveShapeBase)
{
	if (thisObj->isClientObject())
	{

		if (strcmp(thisObj->getDataBlock()->getName(), "TrapDoor")==0)
		{
			std::vector<TGE::TSShapeInstance*>::iterator it = std::find(trapdoorShapeInstances.begin(), trapdoorShapeInstances.end(), thisObj->getTSShapeInstance());
			trapdoorShapeInstances.erase(it);
#ifdef _DEBUG
			std::vector<TGE::ShapeBase*>::iterator it2 = std::find(clientTrapdoors.begin(), clientTrapdoors.end(), thisObj);
			clientTrapdoors.erase(it2);
			TGE::Con::printf("Removing Shape %s to ClientList", thisObj->getIdString());
#endif
		}
		serverToClientSBMap.erase(thisObj->getId());
	}
	origOnRemoveShapeBase(thisObj);
}

ConsoleFunction(GetClientShape,int, 2, 2, "GetClientShape(id)")
{
	int id = atoi(argv[1]);

	auto it = serverToClientSBMap.find(id);

	if (it == serverToClientSBMap.end())
		return id;
	else
		return (*it).second->getId();
}

#ifdef _DEBUG
ConsoleFunction(GetMPCount, int, 1, 1, "GetMPCount()")
{
	return clientPathedInteriors.size();
}

ConsoleFunction(GetMPAt, int, 2, 2, "GetMPAt(int index)")
{
	return clientPathedInteriors.at(atoi(argv[1]))->getId();
}
#endif

PLUGINCALLBACK void preEngineInit(PluginInterface *plugin)
{

}

PLUGINCALLBACK void postEngineInit(PluginInterface *plugin)
{

}

PLUGINCALLBACK void engineShutdown(PluginInterface *plugin)
{

}