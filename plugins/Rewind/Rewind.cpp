#include <cstdio>
#include <TorqueLib/math/mMath.h>
#include <TorqueLib/math/mathUtils.h>
#include <TorqueLib/TGE.h>
#include <TorqueLib/QuickOverride.h>
#include <PluginLoader/PluginInterface.h>
#include "Rewind.h"
#include "RewindApi.h"
#include "StringMath.h"
#include "Logging.h"

// Gets the current thread position of the staticshape
float getThreadPos(int staticshape)
{
	TGE::ShapeBase *s = static_cast<TGE::ShapeBase*>(TGE::Sim::findObject_int(staticshape));
	TGE::TSThread *t = s->getThread1();
	if (t != 0)
		return t->getPos(); //This, is a setter function not a member function
	return 0;
}

// Same thing as above
float getThreadPos(const char* staticshape)
{
	TGE::ShapeBase *s = static_cast<TGE::ShapeBase*>(TGE::Sim::findObject(staticshape));
	TGE::TSThread *t = s->getThread1();
	if (t != 0)
		return t->getPos(); //This, is a setter function not a member function
	return 0;
}

// Gets the dynamic field value of an object
const char* getField(TGE::SimObject* obj, const char* field)
{
	const char* name = TGE::StringTable->insert(field, false);
	return (const_cast<TGE::SimObject*>(obj))->getDataField(name, NULL);
}

// Sets the dynamic field value of an object
void setField(TGE::SimObject* obj, const char* field, const char* value)
{
	const char* name = TGE::StringTable->insert(field, false);
	const char* val = TGE::StringTable->insert(value, false);
	const_cast<TGE::SimObject*>(obj)->setDataField(name, NULL, val);
}

// Calls the hide function for an object
void hide(TGE::SimObject* obj, int val)
{
	static_cast<TGE::ShapeBase*>(obj)->setHidden(val);
}

// Calls the isHidden function for an object
bool getHidden(TGE::SimObject* obj)
{
	return static_cast<TGE::ShapeBase*>(obj)->getHiddenGetter();
}

// Sets the path position of a pathedInterior, implementation copied from TGE source, cause apparently that works
void setPathPosition(SimObjectId pathedInterior, int pos)
{
	DebugPush("Entering setPathPosition");
	TGE::PathedInterior *pServerd = static_cast<TGE::PathedInterior*>(TGE::Sim::findObject_int(pathedInterior));

	TGE::PathedInterior *p = NULL;

	std::map<int, TGE::PathedInterior*>::iterator it = serverToClientPIMap.find(pServerd->getId());
	if (it == serverToClientPIMap.end())
		p = pServerd;
	else
		p = serverToClientPIMap.at(pServerd->getId());

	int pathpos = pos;

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

	if (pathpos < 0)
		pathpos = 0;
	if (pathpos > TGE::gServerPathManager->getPathTotalTime(p->getPathKey()))
		pathpos = TGE::gServerPathManager->getPathTotalTime(p->getPathKey());

	pServerd->setPathPosition(pathpos);
	pServerd->setTargetPosition(pathpos);
	p->setPathPosition(pathpos);
	p->setTargetPosition(pathpos);


	Point3F pathPosPoint;
	TGE::gClientPathManager->getPathPosition(p->getPathKey(), pathpos, pathPosPoint);
	MatrixF mat = p->getTransform();
	mat.setColumn(3, pathPosPoint + p->getOffset());
	p->setTransform(mat);
	p->setMaskBits(0x8 | 0x10);
	DebugPop("Leaving setPathPosition");
}

// Sets the target position of a pathedInterior, same reason as above
void setTargetPosition(SimObjectId pathedInterior, int pos)
{
	DebugPush("Entering setTargetPosition");
	TGE::PathedInterior* pServerd = static_cast<TGE::PathedInterior*>(TGE::Sim::findObject_int(pathedInterior));

	TGE::PathedInterior* p = NULL;

	std::map<int, TGE::PathedInterior*>::iterator it = serverToClientPIMap.find(pServerd->getId());
	if (it == serverToClientPIMap.end())
		p = pServerd;
	else
		p = serverToClientPIMap.at(pServerd->getId());

	pServerd->setTargetPosition(pos);
	p->setTargetPosition(pos);
	DebugPop("Leaving setTargetPosition");
}

// Calls the TorqueScript defined function of the same name
void SetTrapdoorThreadPos(SimObjectId id, float pos,bool isclient)
{
	std::string idstr = std::to_string(id);
	std::string posstr = std::to_string(pos);
	TGE::Con::executef(3, "SetTrapdoorThreadPos", idstr.c_str(), posstr.c_str());
}

// Gets the powerup states at the moment
std::vector<int> GetPowerupTimeStates()
{
	DebugPush("Entering GetPowerupTimeStates");
	TGE::NetConnection* LocalClientConnection = static_cast<TGE::NetConnection*>(TGE::Sim::findObject("LocalClientConnection"));
	TGE::Marble* player = static_cast<TGE::Marble*>(TGE::Sim::findObject(getField(LocalClientConnection,"Player")));
	std::vector<int> PowerupStates;
	if (player != NULL)
	{
		PowerupStates.push_back(atoi(getField(player,"SuperBounceActive")));
		PowerupStates.push_back(atoi(getField(player, "ShockAbsorberActive")));
		PowerupStates.push_back(atoi(getField(player, "GyrocopterActive")));
	}
	else
	{
		PowerupStates.push_back(0);
		PowerupStates.push_back(0);
		PowerupStates.push_back(0);
	}
	DebugPop("Leaving GetPowerupTimeStates");
	return PowerupStates;	
}

// Sets the powerup states
void SetPowerupTimeStates(std::vector<int> PowerupStates, TGE::SimObject* localclientconnectionplayer)
{
	DebugPush("Entering SetPowerupTimeStates");
	if (PowerupStates.size() != 3)
	{
		PowerupStates.clear();
		PowerupStates.push_back(0);
		PowerupStates.push_back(0);
		PowerupStates.push_back(0);
	}
	//char buffer[256];
	//sprintf(buffer, "SuperBounceItem::CustomOnUse(LocalClientConnection.player,%d);ShockAbsorberItem::CustomOnUse(LocalClientConnection.player,%d);HelicopterItem::CustomOnUse(LocalClientConnection.player,%d);", PowerupStates[0], PowerupStates[1], PowerupStates[2]);
	//TGE::Con::evaluatef(buffer);
	executefnmspc("SuperBounceItem", "CustomOnUse", 2, localclientconnectionplayer->getIdString(), TGE::StringTable->insert(StringMath::print(PowerupStates[0]), false));
	executefnmspc("ShockAbsorberItem", "CustomOnUse", 2, localclientconnectionplayer->getIdString(), TGE::StringTable->insert(StringMath::print(PowerupStates[1]), false));
	executefnmspc("HelicopterItem", "CustomOnUse", 2, localclientconnectionplayer->getIdString(), TGE::StringTable->insert(StringMath::print(PowerupStates[2]), false));
	DebugPop("Leaving SetPowerupTimeStates");
}

// Checks if an object can have Rewind implemented
bool IsRewindableType(const char* type)
{
	if (strcmp(type, "StaticShape") == 0 || strcmp(type,"Trigger") == 0 || strcmp(type,"Item") == 0)
		return true;
	return false;
}

// Gets the state of everything in the MissionGroup
void GetMissionState(TGE::SimGroup* group, MissionState* missionstate)
{
	DebugPush("Entering GetMissionState");
	for (int i = 0; i < group->getCount(); i++)
	{
		TGE::SimObject* obj = group->objectList[i];

		const char* type = obj->getClassRep()->getClassName();
		if (strcmp(type, "SimGroup") == 0)
		{
			GetMissionState(static_cast<TGE::SimGroup*>(obj), missionstate);
		}
		else
		{
			// Get the state of the pathedInterior
			if (strcmp(type, "PathedInterior") == 0)
			{
				DebugPrint("Storing MPState");
				MPState state;
				TGE::PathedInterior* pi;
				if (serverToClientPIMap.find(obj->getId()) != serverToClientPIMap.end())
				{
					TGE::PathedInterior* pi = serverToClientPIMap.at(obj->getId());
					state.pathPosition = pi->getPathPosition();
				}
				else
					state.pathPosition = 0;
				state.targetPosition = static_cast<TGE::PathedInterior*>(obj)->getTargetPosition();
				state.pathedInterior = static_cast<TGE::PathedInterior*>(obj);
				missionstate->mpstates.push_back(state);
			} //GetMPState

			// Get the state of items
			if (strcmp(type, "Item") == 0)
			{
				if (strcmp(getField(static_cast<TGE::GameBase*>(obj)->getDataBlock(), "className"), "Gem") == 0)
				{
					DebugPrint("Storing GemState");
					missionstate->gemstates.push_back(getHidden(static_cast<TGE::Item*>(obj)));
				}
				if (strcmp(static_cast<TGE::GameBase*>(obj)->getDataBlock()->getName(), "TimeTravelItem") == 0)
				{
					DebugPrint("Storing TimeTravelState");
					missionstate->ttstates.push_back(getHidden(static_cast<TGE::Item*>(obj)));
				}
				if (strcmp(getField(static_cast<TGE::GameBase*>(obj)->getDataBlock(), "className"), "Gem") != 0 && strcmp(static_cast<TGE::GameBase*>(obj)->getDataBlock()->getName(), "TimeTravelItem") != 0 
#ifdef MBP
					// Theres easter eggs and shit
					&& strcmp(static_cast<TGE::GameBase*>(obj)->getDataBlock()->getName(), "EasterEgg") != 0
#endif
					)
				{
					DebugPrint("Storing PowerupState");
					const char* resetClock = getField(obj, "respawnTime");
					if (strcmp(resetClock, "") == 0)
						missionstate->powerupstates.push_back(0);
					else
						missionstate->powerupstates.push_back(atoi(resetClock));
				}

#ifdef  MBP

				if (strcmp(static_cast<TGE::GameBase*>(obj)->getDataBlock()->getName(), "EasterEgg") == 0)
				{
					missionstate->eggstate = getHidden(obj);
				}

#endif //  MBP
			}
			// Get the sate of staticshapes
			if (strcmp(type, "StaticShape") == 0)
			{
				if (strcmp(getField(static_cast<TGE::GameBase*>(obj)->getDataBlock(), "className"), "Explosive") == 0)
				{
					DebugPrint("Storing LandMineState");
					const char* resetClock = getField(obj, "resetClock");
					if (strcmp(resetClock, "") == 0)
						missionstate->explosivestates.push_back(0);
					else
						missionstate->explosivestates.push_back(atoi(resetClock));
				}

				if (strcmp(static_cast<TGE::GameBase*>(obj)->getDataBlock()->getName(), "TrapDoor") == 0)
				{
					DebugPrint("Storing TrapdoorPosition");
					missionstate->trapdoorpos.push_back(getThreadPos(obj->getId()));

					DebugPrint("Storing TrapdoorDirection");
					auto it = serverToClientSBMap.find(obj->getId());
					missionstate->trapdoordirs.push_back((*it).second->getThread1Forward());

					DebugPrint("Storing TrapdoorOpen");
					const char* opentime = getField(obj, "openTime");
					if (strcmp(opentime, "") == 0)
						missionstate->trapdooropen.push_back(0);
					else
						missionstate->trapdooropen.push_back(atoi(opentime));

					DebugPrint("Storing TrapdoorClose");
					const char* closetime = getField(obj, "closeTime");
					if (strcmp(closetime, "") == 0)
						missionstate->trapdoorclose.push_back(0);
					else
						missionstate->trapdoorclose.push_back(atoi(closetime));
				}
			}

			// Finally we get the states of the user defined objects
			if (IsRewindableType(type))
			{
				DebugPush("Getting RewindableStates (SceneObject)");
				const char* datablock = static_cast<TGE::GameBase*>(obj)->getDataBlock()->getName();

				for (int i = 0; i < rewindManager.rewindableBindings.size(); i++)
				{
					RewindableBindingBase* binding = rewindManager.rewindableBindings[i];

					if (binding->BindingType == Variable)
						continue;

					if (strcmp(binding->BindingNamespace.c_str(), datablock) == 0)
					{
						DebugPrint("Getting RewindableState for %s:%d", binding->BindingNamespace.c_str(), binding->getStorageType());

						int storagetype = binding->getStorageType();

						// Very hacky cause apparently the macros dont work on Mac build
#ifdef WIN32
#define StoreRewindableState(type1,type2) 	RewindableBinding<##type1##>* rewindable = static_cast<RewindableBinding<##type1##>*>(binding); \
						RewindableState<##type1##> state(binding->BindingNamespace); \
						state.value = rewindable->getState(obj); \
						missionstate->rewindable##type2##States.push_back(state);
						if (storagetype == 0)
						{
							StoreRewindableState(int, Int);
						}
						if (storagetype == 1)
						{
							StoreRewindableState(float, Float);
						}
						if (storagetype == 2)
						{
							StoreRewindableState(bool, Bool);
						}
						if (storagetype == 3)
						{
							StoreRewindableState(std::string, String);
						}

#undef StoreRewindableState(type1,type2)
#endif
#ifdef __APPLE__
						if (storagetype == 0)
						{
							RewindableBinding<int>* rewindable = static_cast<RewindableBinding<int>*>(binding);
							RewindableState<int> state(binding->BindingNamespace);
							state.value = rewindable->getState(obj);
							missionstate->rewindableIntStates.push_back(state);
						}
						if (storagetype == 1)
						{
							RewindableBinding<float>* rewindable = static_cast<RewindableBinding<float>*>(binding);
							RewindableState<float> state(binding->BindingNamespace);
							state.value = rewindable->getState(obj);
							missionstate->rewindableFloatStates.push_back(state);
						}
						if (storagetype == 2)
						{
							RewindableBinding<bool>* rewindable = static_cast<RewindableBinding<bool>*>(binding);
							RewindableState<bool> state(binding->BindingNamespace);
							state.value = rewindable->getState(obj);
							missionstate->rewindableBoolStates.push_back(state);
						}
						if (storagetype == 3)
						{
							RewindableBinding<std::string>* rewindable = static_cast<RewindableBinding<std::string>*>(binding);
							RewindableState<std::string> state(binding->BindingNamespace);
							state.value = rewindable->getState(obj);
							missionstate->rewindableStringStates.push_back(state);
						}
#endif

					}
				}

				DebugPop("Leaving Getting RewindableStates");
			}
		}
	}
	DebugPop("Leaving GetMissionState");
}

void SetMissionState(TGE::SimGroup* group, MissionState* missionstate,MissionState* previousstate)
{
	DebugPush("Entering SetMissionState");
	for (int i = 0; i < group->getCount(); i++)
	{
		TGE::SimObject* obj = group->objectList[i];
		const char* type = obj->getClassRep()->getClassName();
		if (strcmp(type, "SimGroup") == 0)
		{
			SetMissionState(static_cast<TGE::SimGroup*>(obj), missionstate,previousstate);
		}
		else
		{
			if (strcmp(type, "PathedInterior") == 0 && missionstate->mpstates.size() != 0)
			{
				// FrameState
				std::vector<MPState>* mpstates = &missionstate->mpstates;

				MPState state = mpstates->at(0);
				mpstates->erase(mpstates->begin());

				// OriginalState
				std::vector<MPState>* prevmpstates = &previousstate->mpstates;

				MPState prevstate = prevmpstates->at(0);
				prevmpstates->erase(prevmpstates->begin());


				DebugPrint("Setting PathedInterior State %d: %f %f", obj->getId(), state.pathPosition, state.targetPosition);

				//if (state.pathPosition != prevstate.pathPosition)
					setPathPosition(obj->getId(), state.pathPosition);
					
					setField(obj, "pathPos", TGE::StringTable->insert(StringMath::print(state.pathPosition), false));

				//if (state.targetPosition != prevstate.targetPosition)
					setField(obj, "targetPos", TGE::StringTable->insert(StringMath::print(state.targetPosition), false));
			}
			if (strcmp(type, "Item") == 0)
			{
				if (strcmp(getField(static_cast<TGE::GameBase*>(obj)->getDataBlock(), "className"), "Gem") == 0)
				{
					DebugPrint("Setting Gem Visibility %d", missionstate->gemstates[0]);

					if (missionstate->gemstates[0] != previousstate->gemstates[0])
						hide(obj, missionstate->gemstates[0]);

					missionstate->gemstates.erase(missionstate->gemstates.begin());
					previousstate->gemstates.erase(previousstate->gemstates.begin());
				}

				if (strcmp(static_cast<TGE::GameBase*>(obj)->getDataBlock()->getName(), "TimeTravelItem") == 0)
				{
					DebugPrint("Setting TimeTravel Visibility %d", missionstate->ttstates[0]);
					if (missionstate->ttstates[0] != previousstate->ttstates[0])
						hide(obj, missionstate->ttstates[0]);
					missionstate->ttstates.erase(missionstate->ttstates.begin());
					previousstate->ttstates.erase(previousstate->ttstates.begin());
				}

				if (strcmp(getField(static_cast<TGE::GameBase*>(obj)->getDataBlock(), "className"), "Gem") != 0 && strcmp(static_cast<TGE::GameBase*>(obj)->getDataBlock()->getName(), "TimeTravelItem") != 0
#ifdef MBP
					&& strcmp(static_cast<TGE::GameBase*>(obj)->getDataBlock()->getName(), "EasterEgg") != 0
#endif										
					)
				{
					int state = missionstate->powerupstates[0];
					DebugPrint("Setting Powerup State %d", state);

					missionstate->powerupstates.erase(missionstate->powerupstates.begin());

					TGE::Sim::cancelEvent(atoi(getField(obj, "respawnSchedule")));
					TGE::Sim::cancelEvent(atoi(getField(obj, "respawnSchedule2")));

					
					
					if (state > 0)
					{
						hide(obj, 1);
						TGE::Con::executef(obj, 4, "startFade", "0", "0", "1");

						char buf2[256];
						sprintf(buf2, " %d.respawnSchedule = %d.schedule(%d, \"hide\", \"false\");%d.respawnSchedule2 = %d.schedule(%d, \"startFade\", 1000, 0, false);", obj->getId(), obj->getId(), state, obj->getId(), obj->getId(), state + 100);
						TGE::Con::evaluatef(buf2);
					}
					else
					{
						hide(obj, 0);
						TGE::Con::executef(obj, 4, "startFade", "0", "0", "0");
					}

					setField(obj, "respawnTime", TGE::StringTable->insert(StringMath::print(state > 0 ? state : 0), false));
					
				}

#ifdef  MBP

				if (strcmp(static_cast<TGE::GameBase*>(obj)->getDataBlock()->getName(), "EasterEgg") == 0)
				{
					hide(obj, missionstate->eggstate);
				}

#endif //  MBP


			}
			if (strcmp(type, "StaticShape") == 0)
			{
				if (strcmp(getField(static_cast<TGE::GameBase*>(obj)->getDataBlock(), "className"), "Explosive") == 0)
				{
					int state = missionstate->explosivestates[0];
					int prevstate = previousstate->explosivestates[0];
					DebugPrint("Setting Mine Visibility %d", state);
					missionstate->explosivestates.erase(missionstate->explosivestates.begin());
					previousstate->explosivestates.erase(previousstate->explosivestates.begin());


					TGE::Sim::cancelEvent(atoi(getField(obj, "resetSchedule")));
					TGE::Sim::cancelEvent(atoi(getField(obj, "resetSchedule2")));

					if (state > 0)
					{
						hide(obj, 1);
						TGE::Con::executef(obj, 4, "startFade", "0", "0", "1");

						char buf2[256];
						sprintf(buf2, " %d.resetSchedule = %d.schedule(%d, \"setDamageState\", \"Enabled\");%d.resetSchedule2 = %d.schedule(%d, \"startFade\", 1000, 0, false);", obj->getId(), obj->getId(), state, obj->getId(), obj->getId(), state);
						TGE::Con::evaluatef(buf2);
					}
					else
					{
						hide(obj, 0);
						TGE::Con::executef(obj, 4, "startFade", "0", "0", "0");
						TGE::Con::executef(obj, 2, "setDamageState", "Enabled");

					}
					setField(obj, "resetClock", TGE::StringTable->insert(StringMath::print(state > 0 ? state : 0),false));
				}
				if (missionstate->trapdoorpos.size() != 0)
				{
					if (strcmp(static_cast<TGE::GameBase*>(obj)->getDataBlock()->getName(), "TrapDoor") == 0)
					{
						DebugPrint("Setting Trapdoor Position %f", missionstate->trapdoorpos[0]);

						if (missionstate->trapdoorpos[0] != previousstate->trapdoorpos[0])
							SetTrapdoorThreadPos(obj->getId(), missionstate->trapdoorpos[0]);

						missionstate->trapdoorpos.erase(missionstate->trapdoorpos.begin());
						previousstate->trapdoorpos.erase(previousstate->trapdoorpos.begin());

						DebugPrint("Setting Trapdoor Direction %d", missionstate->trapdoordirs[0]);
						std::string dirstr = std::to_string(missionstate->trapdoordirs[0]);
						TGE::Con::executef(obj, 3, "setThreadDir", "0", dirstr.c_str());
						TGE::Con::executef(obj, 2, "playThread", "0");
						missionstate->trapdoordirs.erase(missionstate->trapdoordirs.begin());
						previousstate->trapdoordirs.erase(previousstate->trapdoordirs.begin());

						DebugPrint("Setting Trapdoor OpenTime %d", missionstate->trapdooropen[0]);
						//Set Open
						int open = missionstate->trapdooropen[0];
						TGE::Sim::cancelEvent(atoi(getField(obj, "openSchedule")));
						setField(obj, "openTime", TGE::StringTable->insert(StringMath::print(open < 0 ? 0 : open),false));

						if (open > 0)
						{
							setField(obj, "open", "1");
							char buf3[96];
							sprintf(buf3, "%d.openSchedule = schedule(%d,%d,\"Trapdoor_open\",%d);", obj->getId(), open, obj->getId(), obj->getId());
							TGE::Con::evaluatef(buf3);
						}

						missionstate->trapdooropen.erase(missionstate->trapdooropen.begin());

						DebugPrint("Setting Trapdoor CloseTime %d", missionstate->trapdoorclose[0]);
						//Set Close
						int close = missionstate->trapdoorclose[0];
						TGE::Sim::cancelEvent(atoi(getField(obj, "closeSchedule")));

						if (close > 0)
						{
							char buf4[96];
							sprintf(buf4, "%d.closeSchedule = schedule(%d,%d,\"Trapdoor_close\",%d);", obj->getId(), close, obj->getId(), obj->getId());
							TGE::Con::evaluatef(buf4);
							setField(obj, "closeTime", TGE::StringTable->insert(StringMath::print(close),false));
						}
						else
						{
							setField(obj, "open", "0");
							setField(obj, "openTime", "0");
						}

						missionstate->trapdoorclose.erase(missionstate->trapdoorclose.begin());

					}
				}
			}
			if (IsRewindableType(type))
			{
				DebugPrint("Setting RewindableStates (SceneObject)");
				const char* datablock = static_cast<TGE::GameBase*>(obj)->getDataBlock()->getName();

				for (int i = 0; i < rewindManager.rewindableBindings.size(); i++)
				{
					RewindableBindingBase* binding = rewindManager.rewindableBindings[i];

					if (binding->BindingType == Variable)
						continue;

					if (strcmp(binding->BindingNamespace.c_str(), datablock) == 0)
					{
						DebugPush("Setting RewindableState %s::%d", binding->BindingNamespace.c_str(), binding->getStorageType());

						int storagetype = binding->getStorageType();

#ifdef WIN32
#define SetRewindableState(type1,type2)  RewindableState<##type1##> state = missionstate->rewindable##type2##States[0]; \
						missionstate->rewindable##type2##States.erase(missionstate->rewindable##type2##States.begin()); \
						RewindableBinding<##type1##>* rewindable = static_cast<RewindableBinding<##type1##>*>(binding); \
						rewindable->setState(state.value, obj);

						if (storagetype == 0)
						{
							SetRewindableState(int, Int);
						}
						if (storagetype == 1)
						{
							SetRewindableState(float, Float);
						}
						if (storagetype == 2)
						{
							SetRewindableState(bool, Bool);
						}
						if (storagetype == 3)
						{
							SetRewindableState(std::string, String);
						}
#undef SetRewindableState(type1,type2)
#endif
#ifdef __APPLE__
						if (storagetype == 0)
						{
							RewindableState<int> state = missionstate->rewindableIntStates[0];
							missionstate->rewindableIntStates.erase(missionstate->rewindableIntStates.begin());
							RewindableBinding<int>* rewindable = static_cast<RewindableBinding<int>*>(binding);
							rewindable->setState(state.value, obj);
						}
						if (storagetype == 1)
						{
							RewindableState<float> state = missionstate->rewindableFloatStates[0];
							missionstate->rewindableFloatStates.erase(missionstate->rewindableFloatStates.begin());
							RewindableBinding<float>* rewindable = static_cast<RewindableBinding<float>*>(binding);
							rewindable->setState(state.value, obj);
						}
						if (storagetype == 2)
						{
							RewindableState<bool> state = missionstate->rewindableBoolStates[0];
							missionstate->rewindableBoolStates.erase(missionstate->rewindableBoolStates.begin());
							RewindableBinding<bool>* rewindable = static_cast<RewindableBinding<bool>*>(binding);
							rewindable->setState(state.value, obj);
						}
						if (storagetype == 3)
						{
							RewindableState<std::string> state = missionstate->rewindableStringStates[0];
							missionstate->rewindableStringStates.erase(missionstate->rewindableStringStates.begin());
							RewindableBinding<std::string>* rewindable = static_cast<RewindableBinding<std::string>*>(binding);
							rewindable->setState(state.value, obj);
						}
#endif

						DebugPop("Leaving Set RewindableState");
					}
				}

			}

		}
	}
	DebugPop("Leaving SetMissionState");
}

void SetUpPathedInteriors(TGE::SimGroup* group, std::vector<TGE::PathedInterior*>* pi)
{
	DebugPush("Entering SetUpPathedInteriors");
	for (int i = 0; i < group->getCount(); i++)
	{

		TGE::SimObject* obj = group->objectList[i];

		const char* type = obj->getClassRep()->getClassName();
		if (strcmp(type, "SimGroup") == 0)
		{
			SetUpPathedInteriors(static_cast<TGE::SimGroup*>(obj), pi);
		}
		else
		{
			if (strcmp(type, "PathedInterior") == 0)
			{
				TGE::PathedInterior* mp = static_cast<TGE::PathedInterior*>(obj);

				TGE::PathedInterior* pclient = NULL;

				std::map<int, TGE::PathedInterior*>::iterator it = serverToClientPIMap.find(mp->getId());
				if (it == serverToClientPIMap.end())
					pclient = mp;
				else
					pclient = serverToClientPIMap.at(mp->getId());

				pi->push_back(pclient);
			}
		}
	}
	DebugPop("Leaving SetUpPathedInteriors");
}

#ifdef MBP

CheckpointState GetCheckpointState(TGE::Marble* player)
{
	DebugPush("Entering GetCheckpointState");
	CheckpointState cs;
	cs.Obj = std::string(getField(player, "checkPoint"));
	cs.gemCount = atoi(getField(player, "checkPointGemCount"));
	cs.gemStates = std::string(getField(player, "checkPointGemStates"));
	cs.powerup = atoi(getField(player, "checkPointPowerup"));
	cs.gravity = std::string(getField(player, "checkPointGravity"));
	cs.respawnCounter = TGE::Con::getIntVariable("$respawnCounter");
	cs.respawnOffset = std::string(getField(player, "checkPointRespawnOffset"));
	DebugPop("Leaving GetCheckpointState");

	return cs;
}

void SetCheckpointState(TGE::Marble* player, CheckpointState cs)
{
	DebugPush("Entering SetCheckpointState");
	setField(player, "checkPoint", cs.Obj.c_str());
	setField(player, "checkPointGemCount", TGE::StringTable->insert(StringMath::print(cs.gemCount),false));
	setField(player, "checkPointGemStates", cs.gemStates.c_str());
	setField(player, "checkPointPowerup", TGE::StringTable->insert(StringMath::print(cs.powerup),false));
	setField(player, "checkPointGravity", cs.gravity.c_str());
	TGE::Con::setIntVariable("$respawnCounter", cs.respawnCounter);
	setField(player, "checkPointRespawnOffset", cs.respawnOffset.c_str());
	DebugPop("Leaving SetCheckpointState");
}

#endif

Frame GetCurrentFrame(int ms)
{
	DebugPush("Entering GetCurrentFrmae");
	TGE::NetConnection* LocalClientConnection = static_cast<TGE::NetConnection*>(TGE::Sim::findObject("LocalClientConnection"));
	TGE::Marble* player = static_cast<TGE::Marble*>(TGE::Sim::findObject(getField(LocalClientConnection, "Player")));
	TGE::SimGroup* MissionGroup = static_cast<TGE::SimGroup*>(TGE::Sim::findObject("MissionGroup"));
	TGE::SimObject* PlayGui = TGE::Sim::findObject("PlayGui");

	TGE::Con::evaluatef("if (isObject(LocalClientConnection.player.getPowerup())) $CurrentPowerup = LocalClientConnection.player.getPowerup().getID(); else $CurrentPowerup = 0;");
	TGE::Marble* ClientMarble = static_cast<TGE::Marble*>(TGE::Sim::findObject(TGE::Con::executef(1, "ClientMarble")));

	Frame f;
	DebugPrint("Storing Basic Details");
	f.deltaMs = ms;//atoi(getField(PlayGui,".timeDelta"));
	f.elapsedTime = atoi(getField(PlayGui, "totalTime"));
	f.ms = atoi(getField(PlayGui, "elapsedTime"));
	f.position = ClientMarble->getTransform().getPosition();
	f.velocity = ClientMarble->getVelocity();
	f.spin = ClientMarble->getAngularVelocity();
	f.powerup = atoi(TGE::Con::getVariable("$CurrentPowerup"));
	f.timebonus = (int)std::atof(getField(PlayGui, "bonusTime"));
	f.gemcount = atoi(getField(LocalClientConnection, "gemCount"));
	f.gamestate = std::string(TGE::Con::getVariable("$Game::State"));
	f.nextstatetime = atoi(TGE::Con::getVariable("$Game::NextStateTime"));
	f.activepowstates = GetPowerupTimeStates();
	f.gravityDir = std::string(TGE::Con::getVariable("$Game::GravityDir"));
	DebugPrint("Storing MissionState");
	MissionState state;
	GetMissionState(MissionGroup, &state);
	f.mpstates = state.mpstates;
	f.gemstates = state.gemstates;
	f.ttstates = state.ttstates;
	f.powerupstates = state.powerupstates;
	f.trapdoorpos = state.trapdoorpos;
	f.trapdoordirs = state.trapdoordirs;
	f.trapdooropen = state.trapdooropen;
	f.trapdoorclose = state.trapdoorclose;
	f.lmstates = state.explosivestates;
	f.rewindableSOIntStates = state.rewindableIntStates;
	f.rewindableSOFloatStates = state.rewindableFloatStates;
	f.rewindableSOBoolStates = state.rewindableBoolStates;
	f.rewindableSOStringStates = state.rewindableStringStates;

#ifdef MBP
	DebugPrint("Getting MBP Feature States");
	TeleportState ts;
	ts.teleportDelay = TGE::Con::getIntVariable("$teleportTimer");
	ts.destination = std::string(TGE::Con::getVariable("$teleportDestination"));
	ts.teleportCounter = TGE::Con::getBoolVariable("$teleportCounter");
	f.teleportState = ts;

	f.checkpointState = GetCheckpointState(player);
	f.eggstate = state.eggstate;
#endif // MBP

	DebugPrint("Getting Rewindable States (Variable)");
	for (int i = 0; i < rewindManager.rewindableBindings.size(); i++)
	{
		RewindableBindingBase* binding = rewindManager.rewindableBindings[i];
		int type = binding->getStorageType();

		if (binding->BindingType == SceneObject) //Its not time for that yet
			continue;

		DebugPrint("Getting RewindableState %s::%d", binding->BindingNamespace.c_str(), binding->getStorageType());

#ifdef WIN32
#define StoreRSState(type1,type2)  RewindableState<##type1##> rs(binding->BindingNamespace); \
		rs.value = static_cast<RewindableBinding<##type1##>*>(binding)->getState(); \
		f.rewindable##type2##States.push_back(rs);

		if (type == 0)
		{
			StoreRSState(int, Int);
		}
		else if (type == 1)
		{
			StoreRSState(float, Float);
		}
		else if (type == 2)
		{
			StoreRSState(bool, Bool);
		}
		else if (type == 3)
		{
			StoreRSState(std::string, String);
		}

#undef StoreRSState(type1,type2)
#endif

#ifdef __APPLE__
		if (type == 0)
		{
			RewindableState<int> rs(binding->BindingNamespace);
			rs.value = static_cast<RewindableBinding<int>*>(binding)->getState();
			f.rewindableIntStates.push_back(rs);
		}
		else if (type == 1)
		{
			RewindableState<float> rs(binding->BindingNamespace);
			rs.value = static_cast<RewindableBinding<float>*>(binding)->getState();
			f.rewindableFloatStates.push_back(rs);
		}
		else if (type == 2)
		{
			RewindableState<bool> rs(binding->BindingNamespace);
			rs.value = static_cast<RewindableBinding<bool>*>(binding)->getState();
			f.rewindableBoolStates.push_back(rs);
		}
		else if (type == 3)
		{
			RewindableState<std::string> rs(binding->BindingNamespace);
			rs.value = static_cast<RewindableBinding<std::string>*>(binding)->getState();
			f.rewindableStringStates.push_back(rs);
		}
#endif

	}
	DebugPop("Leaving GetCurrentFrame");
	return f;
}

void RewindGhost(Frame* f)
{
	DebugPush("Entering RewindGhost");
	if (f == NULL)
		f = &previousGhostFrame;
	else
		previousGhostFrame = *f;

	TGE::ShapeBase* GhostMarble = static_cast<TGE::ShapeBase*>(TGE::Sim::findObject("GhostMarble"));
	MatrixF t = GhostMarble->getTransform();
	
	AngAxisF a = AngAxisF(t);
	Point3F normvec = f->spin.toPoint3F();
	normvec /= sqrt(normvec.x * normvec.x + normvec.y * normvec.y + normvec.z * normvec.z);
	AngAxisF ret = AngAxisF(QuatF().mul(QuatF(a.axis, a.angle), QuatF(normvec, -(f->spin.len() * atoi(getField(TGE::Sim::findObject("PlayGui"), "timeDelta")) * 0.001))));

	MatrixF t2 = *ret.setMatrix(&t);

	t2.setPosition(f->position.toPoint3F());

	GhostMarble->setTransformMember(t2);
	GhostMarble->setTransform(t2);
	GhostMarble->setTransformVirt(t2);
	DebugPop("Leaving RewindGhost");
}

void RewindFrame(Frame* f)
{
	DebugPush("Entering RewindFrame");
	TGE::NetConnection* LocalClientConnection = static_cast<TGE::NetConnection*>(TGE::Sim::findObject("LocalClientConnection"));
	TGE::SimGroup* MissionGroup = static_cast<TGE::SimGroup*>(TGE::Sim::findObject("MissionGroup"));
	TGE::SimObject* PlayGui = TGE::Sim::findObject("PlayGui");

	int totalTime = atoi(getField(PlayGui, "totalTime"));

	// Well get the moving platforms stored
	if (rewindManager.pathedInteriors == NULL)
	{
		DebugPrint("Setting up PathedInteriors");
		rewindManager.pathedInteriors = new std::vector<TGE::PathedInterior*>();
		SetUpPathedInteriors(MissionGroup, rewindManager.pathedInteriors);
	}

	setField(LocalClientConnection,"isOOB", "0");

	TGE::Marble* player = static_cast<TGE::Marble*>(TGE::Sim::findObject(getField(LocalClientConnection,"Player")));
	player->setOOB(0);
	TGE::Sim::cancelEvent(atoi(getField(LocalClientConnection,"respawnSchedule")));

	float rewindDelta = atoi(getField(TGE::Sim::findObject("PlayGui"),"timeDelta"));

	Frame* framedata = NULL;

	DebugPrint("Obtaining Frame to rewind to");
	if (rewindManager.getFrameCount() <= 1)
	{
		DebugPrint("One or no frames left");
		if (rewindManager.getFrameCount() != 0)
			framedata = new Frame(rewindManager.popFrame(false));
		else
			framedata = NULL;
	}
	else
	{

		if (atoi(TGE::Con::getVariable("$pref::Rewind::SyncSpeed")) || atof(TGE::Con::getVariable("$pref::Rewind::TimeScale")) != 1)
		{
			rewindDelta *= atof(TGE::Con::getVariable("$pref::Rewind::TimeScale"));
			DebugPrint("Altered Delta %c", rewindDelta);
			framedata = rewindManager.getNextRewindFrame(rewindDelta);

			if (framedata == NULL)
				framedata = &previousFrame;
		}
		else
		{
			DebugPrint("Unaltered Delta");
			if (f == NULL)
				framedata = new Frame(rewindManager.popFrame(false));
			else
				framedata = f;

			previousFrame = *framedata;
		}
		if (framedata != NULL)
			previousFrame = *framedata;
		else
			framedata = &previousFrame;
	}
	if (framedata != NULL)
		previousFrame = *framedata;
	else
		framedata = &previousFrame;

	DebugPrint("Setting TimeBonus %d",framedata->timebonus);
	TGE::Con::setIntVariable("$Rewind::TimeBonus", framedata->timebonus);

	DebugPrint("Setting Time %d",framedata->ms);
	TGE::Con::executef(PlayGui, 2, "setTime", TGE::StringTable->insert(StringMath::print(framedata->ms), false));
	setField(PlayGui, "totalTime", TGE::StringTable->insert(StringMath::print(framedata->elapsedTime), false));


	TGE::Marble* ClientMarble = static_cast<TGE::Marble*>(TGE::Sim::findObject(TGE::Con::executef(1, "ClientMarble")));

	DebugPrint("Setting Marble Position %f %f %f", framedata->position.x, framedata->position.y, framedata->position.z);
	MatrixF cmtransform = ClientMarble->getTransform();
	cmtransform.setPosition(framedata->position.toPoint3F());

	DebugPrint("Setting Marble Velocity %f %f %f", framedata->velocity.x, framedata->velocity.y, framedata->velocity.z);
	if (!TGE::Con::getBoolVariable("$Replay::Paused"))
		ClientMarble->setVelocity(framedata->velocity);
	else
		ClientMarble->setVelocity(Point3D(0, 0, 0));

	DebugPrint("Setting Marble Angular Velocity %f %f %f", framedata->spin.x, framedata->spin.y, framedata->spin.z);
	ClientMarble->setAngularVelocity(framedata->spin);
	ClientMarble->setTransformMember(cmtransform);
	ClientMarble->setTransform(cmtransform);
	ClientMarble->setTransformVirt(cmtransform);
	player->setTransformMember(cmtransform);
	player->setTransform(cmtransform);
	player->setTransformVirt(cmtransform);
	player->setVelocity(ClientMarble->getVelocity());
	player->setAngularVelocity(ClientMarble->getAngularVelocity());
	player->setCameraYaw(ClientMarble->getCameraYaw());
	player->setCameraPitch(ClientMarble->getCameraPitch());

	MissionState state;

#ifdef MBP
	DebugPrint("Setting MBP Feature States");

	if (framedata->teleportState.teleportDelay > 0)
	{
		TGE::Con::executef(3, "ManualTeleport", TGE::StringTable->insert(StringMath::print(framedata->teleportState.teleportDelay), false), framedata->teleportState.destination.c_str());
	}
	else
		TGE::Con::executef(1, "CancelTeleport");

	SetCheckpointState(player, framedata->checkpointState);

	state.eggstate = framedata->eggstate;

#endif


	state.mpstates = framedata->mpstates;

	state.gemstates = framedata->gemstates;

	state.ttstates = framedata->ttstates;

	state.powerupstates = framedata->powerupstates;

	state.explosivestates = framedata->lmstates;

	state.rewindableIntStates = framedata->rewindableSOIntStates;
	state.rewindableFloatStates = framedata->rewindableSOFloatStates;
	state.rewindableBoolStates = framedata->rewindableSOBoolStates;
	state.rewindableStringStates = framedata->rewindableSOStringStates;


	DebugPrint("Setting PowerupStates");
	for (auto& it : framedata->activepowstates)
		DebugPrint("%d", it);

	SetPowerupTimeStates(framedata->activepowstates, player);

	DebugPrint("Setting GravityDir %s", framedata->gravityDir.c_str());
	TGE::Con::setVariable("$Game::GravityDir", TGE::StringTable->insert(framedata->gravityDir.c_str(), true));

	DebugPrint("Setting NextStateTime %d", framedata->nextstatetime);
	TGE::Con::setIntVariable("$Game::NextStateTime", framedata->nextstatetime);

	DebugPrint("Setting GemCount %d", framedata->gemcount);
	setField(LocalClientConnection, "gemCount", TGE::StringTable->insert(StringMath::print(framedata->gemcount),false));

	DebugPrint("Setting GameState %s", framedata->gamestate.c_str());
	TGE::Con::executef(2, "setRewindGameState", framedata->gamestate.c_str());
	TGE::Con::executef(PlayGui, 2, "setGemCount", TGE::StringTable->insert(StringMath::print(framedata->gemcount), false));
	TGE::Con::executef(2, "setGravityDirection", framedata->gravityDir.c_str());

	if (framedata->trapdoorpos.size() != 0)
	{
		state.trapdoorpos = framedata->trapdoorpos;
		state.trapdoordirs = framedata->trapdoordirs;
		state.trapdooropen = framedata->trapdooropen;
		state.trapdoorclose = framedata->trapdoorclose;
	}

	DebugPrint("Setting MissionState");

	MissionState prevMissionState;
	GetMissionState(MissionGroup, &prevMissionState);

	SetMissionState(MissionGroup, &state,&prevMissionState);

	DebugPrint("Setting RewindableStates (Variable)");
	for (int i = 0; i < rewindManager.rewindableBindings.size(); i++)
	{
		RewindableBindingBase* binding = rewindManager.rewindableBindings[i];

		if (binding->BindingType == SceneObject) //Its not time for that yet
			continue;

		DebugPrint("Setting RewindableState %s::%d", binding->BindingNamespace.c_str(), binding->getStorageType());

		int storagetype = binding->getStorageType();

#ifdef WIN32
#define SetRewindableState(type1,type2)  RewindableState<##type1##> state = framedata->rewindable##type2##States[0]; \
						framedata->rewindable##type2##States.erase(framedata->rewindable##type2##States.begin()); \
						RewindableBinding<##type1##>* rewindable = static_cast<RewindableBinding<##type1##>*>(binding); \
						rewindable->setState(state.value);

		if (storagetype == 0)
		{
			SetRewindableState(int, Int);
		}
		if (storagetype == 1)
		{
			SetRewindableState(float, Float);
		}
		if (storagetype == 2)
		{
			SetRewindableState(bool, Bool);
		}
		if (storagetype == 3)
		{
			SetRewindableState(std::string, String);
		}
#undef SetRewindableState(type1,type2)
#endif
#ifdef __APPLE__
		if (storagetype == 0)
		{
			RewindableState<int> state = framedata->rewindableIntStates[0];
			framedata->rewindableIntStates.erase(framedata->rewindableIntStates.begin());
			RewindableBinding<int>* rewindable = static_cast<RewindableBinding<int>*>(binding);
			rewindable->setState(state.value);
		}
		if (storagetype == 1)
		{
			RewindableState<float> state = framedata->rewindableFloatStates[0];
			framedata->rewindableFloatStates.erase(framedata->rewindableFloatStates.begin());
			RewindableBinding<float>* rewindable = static_cast<RewindableBinding<float>*>(binding);
			rewindable->setState(state.value);
		}
		if (storagetype == 2)
		{
			RewindableState<bool> state = framedata->rewindableBoolStates[0];
			framedata->rewindableBoolStates.erase(framedata->rewindableBoolStates.begin());
			RewindableBinding<bool>* rewindable = static_cast<RewindableBinding<bool>*>(binding);
			rewindable->setState(state.value);
		}
		if (storagetype == 3)
		{
			RewindableState<std::string> state = framedata->rewindableStringStates[0];
			framedata->rewindableStringStates.erase(framedata->rewindableStringStates.begin());
			RewindableBinding<std::string>* rewindable = static_cast<RewindableBinding<std::string>*>(binding);
			rewindable->setState(state.value);
		}
#endif

	}

	DebugPrint("Setting Powerup %d", framedata->powerup);
	TGE::Con::executef(player, 2, "setPowerup", TGE::StringTable->insert(StringMath::print(framedata->powerup), true));
	if (framedata != &previousFrame)
		deleteSafe(framedata);
	DebugPop("Leaving RewindFrame");
}

void StoreCurrentFrame(int ms)
{
	DebugPush("Entering StoreCurrentFrame");
	DebugPrint("Storing Frame %d",ms);
	rewindManager.pushFrame(GetCurrentFrame(ms));
	DebugPop("Leaving StoreCurrentFrame");
}

void CallOnRewindEvent()
{
	DebugPush("Entering CallOnRewindEvent");
	for (auto& binding : rewindManager.rewindableBindings)
	{
		if (binding->BindingType == Variable)
		{
			binding->onRewind();
		}
	}
	TGE::SimGroup* MissionGroup = static_cast<TGE::SimGroup*>(TGE::Sim::findObject("MissionGroup"));
	CallOnRewindEventForSceneObjectBinding(MissionGroup);
	DebugPop("Leaving CallOnRewindEvent");
}

void CallOnRewindEventForSceneObjectBinding(TGE::SimGroup* group)
{
	DebugPush("Entering CallOnRewindEventForSceneObjectBinding");
	for (int i = 0; i < group->getCount(); i++)
	{
		TGE::SimObject* obj = group->objectList[i];

		const char* type = obj->getClassRep()->getClassName();
		if (strcmp(type, "SimGroup") == 0)
		{
			CallOnRewindEventForSceneObjectBinding(static_cast<TGE::SimGroup*>(obj));
		}

		if (IsRewindableType(type))
		{
			const char* datablock = static_cast<TGE::GameBase*>(obj)->getDataBlock()->getName();

			for (int i = 0; i < rewindManager.rewindableBindings.size(); i++)
			{
				RewindableBindingBase* binding = rewindManager.rewindableBindings[i];

				if (binding->BindingType == Variable)
					continue;

				if (strcmp(binding->BindingNamespace.c_str(), datablock) == 0)
				{
					binding->onRewind(obj);
				}
			}
		}
	}
	DebugPop("Leaving CallOnRewindEventForSceneObjectBinding");
}

