#include "RewindManager.h"
#include "frame.h"
#include "RewindApi.h"
#include <zlib.h>
#include "StringMath.h"
#include <map>
#include <thread>
#include "MemoryStream.h"


std::vector<std::string> SplitStringDelim(std::string str, char delim);

void write_list_mpstates(std::vector<MPState> list, MemoryStream* f)
{
	std::string exportstr = std::string("[");
	for (int i = 0; i < list.size(); i++)
	{
		char buf[64];
		sprintf(buf, "%f,%f", list[i].pathPosition, list[i].targetPosition);
		exportstr += std::string(buf);
		if (i != list.size() - 1)
			exportstr += std::string(";");
	}
	exportstr += std::string("]");

	f->writeString(exportstr);;
}

template<typename T>
void write_vector(std::vector<T> list, MemoryStream* f)
{
	int c = list.size();
	f->writeInt32(c);

	for (int i = 0; i < list.size(); i++)
	{
		f->write<T>(list[i]);
	}
}

template<typename T>
std::vector<T> read_vector(MemoryStream* f)
{
	int count = f->readInt32();

	std::vector<T> vec;

	for (int i = 0; i < count; i++)
	{
		T elem = f->read<T>();
		vec.push_back(elem);
	}

	return vec;
}

template<typename T>
void write_vector_rewindable(std::vector<RewindableState<T>> list, MemoryStream* f)
{
	int c = list.size();
	f->writeInt32(c);

	for (int i = 0; i < list.size(); i++)
	{
		RewindableState<T> elem = list[i];
		elem.write(f);
	}
}

template<typename T>
std::vector<RewindableState<T>> read_vector_rewindable(MemoryStream* f)
{
	int c = f->readInt32();

	std::vector<RewindableState<T>> vec;

	for (int i = 0; i < c; i++)
	{
		vec.push_back(RewindableState<T>::read(f));
	}

	return vec;
}

std::vector<int> read_list_int(MemoryStream* f)
{
	std::string list = f->readString();
	list.erase(list.begin());
	list.erase(list.end());

	std::vector<std::string> elemlist = SplitStringDelim(list, ',');

	std::vector<int> out;

	for (int i = 0; i < elemlist.size(); i++)
		out.push_back(atoi(elemlist[i].c_str()));

	return out;
}

std::vector<int> read_list_powerupstates(MemoryStream* f)
{
	std::string list = f->readString();
	list.erase(list.begin());
	list.erase(list.end());

	std::vector<std::string> elemlist = SplitStringDelim(list, ';');

	std::vector<int> out;

	for (int i = 0; i < elemlist.size(); i++)
		out.push_back(atoi(elemlist[i].c_str()));

	return out;
}

std::vector<float> read_list_float(MemoryStream* f)
{
	std::string list = f->readString();
	list.erase(list.begin());
	list.erase(list.end());

	std::vector<std::string> elemlist = SplitStringDelim(list, ',');

	std::vector<float> out;

	for (int i = 0; i < elemlist.size(); i++)
		out.push_back(atof(elemlist[i].c_str()));

	return out;
}

std::vector<MPState> read_list_mpstates(MemoryStream* f)
{
	std::string list = f->readString();
	list.erase(list.begin());
	list.erase(list.end());
	std::vector<std::string> states = SplitStringDelim(list, ';');

	std::vector<MPState> out;

	for (int i = 0; i < states.size(); i++)
	{
		MPState s;
		sscanf(states[i].c_str(), "%f,%f", &s.pathPosition, &s.targetPosition);
		out.push_back(s);
	}

	return out;

}

std::vector<std::string> SplitStringDelim(std::string str,char delim)
{
	const char* cstr = str.c_str();

	char* inputstr = (char*)malloc(strlen(cstr) + 1);
	strcpy(inputstr, cstr);

	std::vector<std::string> out;

	char* splitstr = strtok(inputstr, &delim);

	if (splitstr != NULL)
	{
		while (splitstr != NULL)
		{
			out.push_back(std::string(splitstr));
			splitstr = strtok(NULL, &delim);
		}
	}

	return out;
}

template<typename T>
std::vector<T> InterpolateList(std::vector<T> one, std::vector<T> two, float ratio)
{
	std::vector<T> out;
	for (int i = 0; i < fmin(one.size(),two.size()); i++)
		out.push_back(one[i] + (two[i] - one[i]) * ratio);
	return out;
}

float truncprec(float num, int prec)
{
	return truncf(num * powf(10, prec)) / powf(10, prec);
}

std::vector<MPState> InterpolateMPStates(std::vector<MPState> one, std::vector<MPState> two,std::vector<TGE::PathedInterior*> interiors, float ratio, float deltams)
{
	std::vector<MPState> out;
	for (int i = 0; i < interiors.size(); i++)
	{
		MPState s;
		TGE::PathedInterior* pClient = interiors[i];

		float proposedPosition = one[i].pathPosition + (two[i].pathPosition - one[i].pathPosition) * ratio;

		int tottime = TGE::gServerPathManager->getPathTotalTime(pClient->getPathKey());

		s.targetPosition = (ratio > 0.5) ? two[i].targetPosition : one[i].targetPosition;
		s.pathPosition = 0;
		s.pathedInterior = NULL;

		if (s.targetPosition == -1 || s.targetPosition == -2) //Interpolating these ones is weird, they make the targetPosition a periodic value
		{
			bool isReplay = TGE::Con::getBoolVariable("$Rewind::IsReplay"); // Check the order of the frames

			if (isReplay)
			{
				if (s.targetPosition == -1)
				{
					if (two[i].pathPosition < one[i].pathPosition)
					{
						s.pathPosition = one[i].pathPosition + (two[i].pathPosition - one[i].pathPosition + tottime) * ratio;
						while (s.pathPosition > tottime)
							s.pathPosition -= tottime;
					}
					else
						s.pathPosition = proposedPosition;
				}
				else if (s.targetPosition == -2)
				{
					if (one[i].pathPosition < two[i].pathPosition)
					{
						s.pathPosition = tottime + one[i].pathPosition + (two[i].pathPosition - one[i].pathPosition - tottime) * ratio;
						while (s.pathPosition > tottime)
							s.pathPosition -= tottime;
					}
					else
						s.pathPosition = proposedPosition;
				}

			}
			else
			{
				if (s.targetPosition == -1)
				{
					if (one[i].pathPosition < two[i].pathPosition)
					{
						s.pathPosition = tottime + one[i].pathPosition + (two[i].pathPosition - one[i].pathPosition - tottime) * ratio;
						while (s.pathPosition > tottime)
							s.pathPosition -= tottime;
					}
					else
						s.pathPosition = proposedPosition;
				}
				else if (s.targetPosition == -2)
				{
					if (two[i].pathPosition < one[i].pathPosition)
					{
						s.pathPosition = one[i].pathPosition + (two[i].pathPosition - one[i].pathPosition + tottime) * ratio;
						while (s.pathPosition > tottime)
							s.pathPosition -= tottime;
					}
					else
						s.pathPosition = proposedPosition;
				}
			}

		}
		else
		{
			s.pathPosition = proposedPosition;
		}

		s.targetPosition = (ratio > 0.5) ? two[i].targetPosition : one[i].targetPosition;//one[i].targetPosition + (two[i].targetPosition - one[i].targetPosition) * ratio;
		out.push_back(s);
	}
	return out;
}

float InterpolateNextStateTimer(Frame one, Frame two, float ratio)
{
	bool isReplay = TGE::Con::getBoolVariable("$Rewind::IsReplay"); // Check the order of the frames
	// two > one for normal rewind aka two is older than one
	if (!isReplay)
	{
		if (two.gamestate == "Start")
		{
			if (one.gamestate == "Start")
				return mLerp(one.nextstatetime, two.nextstatetime, ratio);
			if (one.gamestate == "Ready")
				return (ratio < 0.5) ? mLerp(one.nextstatetime + 500, two.nextstatetime, ratio) - 500 : mLerp(one.nextstatetime + 500, two.nextstatetime, ratio);
		}
		else if (two.gamestate == "Ready")
		{
			if (one.gamestate == "Ready")
				return mLerp(one.nextstatetime, two.nextstatetime, ratio);
			if (one.gamestate == "Set")
				return (ratio < 0.5) ? mLerp(one.nextstatetime + 1500, two.nextstatetime, ratio) - 1500 : mLerp(one.nextstatetime + 1500, two.nextstatetime, ratio);
		}
		else if (two.gamestate == "Set")
		{
			if (one.gamestate == "Set")
				return mLerp(one.nextstatetime, two.nextstatetime, ratio);
			if (one.gamestate == "Go")
				return (ratio < 0.5) ? mLerp(one.nextstatetime + 1500, two.nextstatetime, ratio) - 1500 : mLerp(one.nextstatetime + 1500, two.nextstatetime, ratio);
		}
		else if (two.gamestate == "Go")
		{
			if (one.gamestate == "Go")
				return mLerp(one.nextstatetime, two.nextstatetime, ratio);
			if (one.gamestate == "Play")
				return (ratio < 0.5) ? mLerp(one.nextstatetime + 2000, two.nextstatetime, ratio) - 2000 : mLerp(one.nextstatetime + 2000, two.nextstatetime, ratio);
		}
		else
			return mLerp(one.nextstatetime, two.nextstatetime, ratio);
	}
	else
		return mLerp(one.nextstatetime, two.nextstatetime, ratio);
}

template<typename T>
bool CompareListEquality(std::vector<T> one, std::vector<T> two)
{
	//if (one.size() != two.size())
	//	return false;

	for (int i = 0; i < one.size(); i++)
	{
		if (one[i] != two[i])
			return false;
	}
	return true;

}

#ifdef MBP
bool CheckForTeleport(TeleportState one, TeleportState two)
{
	return one.teleportCounter != two.teleportCounter;
}

std::string ChooseNonEmptyString(std::string one, std::string two)
{
	if (strcmp(one.c_str(), "") == 0)
	{
		return two;
	}
	return one;
}

TeleportState InterpolateTeleportState(TeleportState one, TeleportState two, float ratio)
{
	TeleportState ret;
	if (!CheckForTeleport(one, two))
		ret.teleportDelay = mLerp(one.teleportDelay, two.teleportDelay, ratio);
	else
		ret.teleportDelay = (ratio > 0.5) ? two.teleportDelay : one.teleportDelay;
	ret.destination = (ret.teleportDelay > 0) ? ChooseNonEmptyString(one.destination, two.destination) : std::string(); //(ratio > 0.5) ? two.destination : one.destination;
	ret.teleportCounter = (ratio > 0.5) ? two.teleportCounter : one.teleportCounter;
	return ret;
}

bool CheckForRespawn(CheckpointState one, CheckpointState two)
{
	return one.respawnCounter != two.respawnCounter;
}
#endif

RewindManager::RewindManager()
{
}


RewindManager::~RewindManager()
{
}


void RewindManager::pushFrame(Frame f)
{
	Frames.push_back(f);
}


Frame RewindManager::popFrame(bool peek)
{
	if (peek)
		return Frames.back();
	Frame f = Frames.back();
	Frames.pop_back();
	return f;
}


Frame RewindManager::getFrameAt(int index)
{
	return Frames.at(index);
}

int RewindManager::getFrameCount()
{
	return Frames.size();
}

void RewindManager::save(std::string path)
{
	DebugPush("Entering RewindManager::save");
	if (Frames.size() != 0) //We dun wanna save empty files
		{
			FILE *fMain;
			fMain = fopen(path.c_str(), "wb");
			TGE::Con::printf("Saving replay to %s", path.c_str());
			TGE::Con::printf("Frames: %d", Frames.size());
#ifdef  MBP
			char version = 12;
#else
			char version = 11;
#endif
			fwrite(&version, sizeof(char), 1, fMain); //Yeah have to write the version

			TGE::Con::printf("Compressing Replay");
			MemoryStream m;

			int framecount = Frames.size();

			m.writeInt32(framecount);
			m.writeString(replayMission);
			m.writeString(game);
			while (Frames.size() > 0)
			{
				Frame frame = Frames.at(Frames.size() - 1);

				m.writeInt32(frame.ms);
				m.writeInt32(frame.deltaMs);
				m.writeDouble(frame.position.x);
				m.writeDouble(frame.position.y);
				m.writeDouble(frame.position.z);
				m.writeDouble(frame.velocity.x);
				m.writeDouble(frame.velocity.y);
				m.writeDouble(frame.velocity.z);
				m.writeDouble(frame.spin.x);
				m.writeDouble(frame.spin.y);
				m.writeDouble(frame.spin.z);
				m.writeInt32(frame.powerup);
				m.writeInt32(frame.timebonus);
				write_list_mpstates(frame.mpstates, &m);
				m.writeInt32(frame.gemcount);
				write_vector(frame.gemstates, &m);
				write_vector(frame.ttstates, &m);
				write_vector(frame.powerupstates, &m);
				m.writeString(frame.gamestate);
				write_vector(frame.lmstates, &m);
				m.writeInt32(frame.nextstatetime);
				write_vector(frame.activepowstates, &m);
				m.writeString(frame.gravityDir);
				write_vector(frame.trapdoordirs, &m);
				write_vector(frame.trapdooropen, &m);
				write_vector(frame.trapdoorclose, &m);
				write_vector(frame.trapdoorpos, &m);
#ifdef  MBP
				m.writeInt32(frame.teleportState.teleportDelay);
				m.writeString(frame.teleportState.destination);
				m.writeInt32(frame.teleportState.teleportCounter);
				m.writeBool(frame.eggstate);
#endif //  MBP
				write_vector_rewindable(frame.rewindableIntStates, &m);
				write_vector_rewindable(frame.rewindableFloatStates, &m);
				write_vector_rewindable(frame.rewindableBoolStates, &m);
				write_vector_rewindable(frame.rewindableStringStates, &m);

				write_vector_rewindable(frame.rewindableSOIntStates, &m);
				write_vector_rewindable(frame.rewindableSOFloatStates, &m);
				write_vector_rewindable(frame.rewindableSOBoolStates, &m);
				write_vector_rewindable(frame.rewindableSOStringStates, &m);

				Frames.pop_back();
			}
			auto size = m.length();
			auto sz = compressBound(size + 1);
			Bytef* mem = new Bytef[sz + 1];
			compress(mem, &sz, m.getBuffer(), m.length());
			TGE::Con::printf("Completed Compression:%d bytes", sz);
			fwrite(&size, sizeof(unsigned long), 1, fMain); //We gotta write the size of uncompressed replay so that we can allocate enough memory to uncompress the compressed replay
			fwrite(mem, 1, sz, fMain);
			fclose(fMain); //DONE
			delete[] mem;
		}
	DebugPop("Leaving RewindManager::save");
}

std::string RewindManager::load(std::string path,bool isGhost)
{
	DebugPush("Entering RewindManager::load(%s,%d)", path.c_str(), isGhost);
	Frames.clear();
	this->totalTime = 0;

	clearSaveStates();

	FILE *f;


#ifdef  __APPLE__
	std::replace(path.begin(), path.end(), '\\', '/');
#endif //  __APPLE__

	f = fopen(path.c_str(), "rb");

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);
	uint8_t* buffer = new uint8_t[size + 1];
	fread(buffer, sizeof(uint8_t), size, f);
	fclose(f);

	MemoryStream m;
	m.createFromBuffer(buffer, size);
	fclose(f);

	replayPath = path;

	char version = m.readChar();

	unsigned long uncompressedSize = 52428800; //max uncompressed data size - 50mb, bad idea but replays prob wont go over this
	if (version >= 3)
		uncompressedSize = m.readInt32();

	replayMission = std::string("[null]");

	if (version >= 2)
	{
		TGE::Con::printf("Uncompressing Replay");

		uint8_t* buf = &m.getBuffer()[m.tell()];
		uint64_t size = m.length();
		size -= 1; // one byte used by version
		if (version >= 3)
			size -= sizeof(unsigned long);

		uint8_t* uncompressed = new uint8_t[uncompressedSize + 1];

		uncompress((Bytef*)uncompressed, &uncompressedSize, (Bytef*)buf, size);

		m.createFromBuffer(uncompressed, uncompressedSize);


	}
	int framecount = m.readInt32();
	if (version >= 4)
		replayMission = m.readString();
	if (version >= 10)
	{
		std::string replaygame = m.readString();
		if (replaygame != game)
			return replayMission; //ERR WRONG REPLAY GAME
	}


	int i;
	for (i = 0; i < framecount; i++)
	{
		int ms, deltaMs;
		ms = m.readInt32();
		deltaMs = m.readInt32();
		//TGE::Con::printf("Loaded Time: %d %d", ms,deltaMs);
		double px, py, pz, vx, vy, vz, sx, sy, sz;
		px = m.readDouble();
		py = m.readDouble();
		pz = m.readDouble();
		Point3D position = Point3D(px, py, pz);
		//TGE::Con::printf("Loaded Pos: %lf %lf %lf", px, py, pz);
		vx = m.readDouble();
		vy = m.readDouble();
		vz = m.readDouble();
		Point3D velocity = Point3D(vx, vy, vz);
		//TGE::Con::printf("Loaded Vel: %lf %lf %lf", vx, vy, vz);
		sx = m.readDouble();
		sy = m.readDouble();
		sz = m.readDouble();
		Point3D spin = Point3D(sx, sy, sz);
		//TGE::Con::printf("Loaded Spn: %lf %lf %lf", sx, sy, sz);
		int pow, timebonus, gemcount;
		pow = m.readInt32();
		timebonus = m.readInt32();
		//TGE::Con::printf("Loaded Pow And TB: %d %d", pow,timebonus);
		std::vector<MPState> mpstates = read_list_mpstates(&m);
		//TGE::Con::printf("Loaded MPStates: %s", mpstates.c_str());
		gemcount = m.readInt32();
		//TGE::Con::printf("Loaded GC: %d", gemcount);
		std::vector<int> gemstates, ttstates, powerupstates, lmstates, activepowstates;

		if (version >= 7)
		{
			gemstates = read_vector<int>(&m);
			ttstates = read_vector<int>(&m);
			powerupstates = read_vector<int>(&m);
		}
		else
		{
			gemstates = read_list_int(&m);
			//TGE::Con::printf("Loaded MPStates: %s", mpstates.c_str());
			ttstates = read_list_int(&m);
			//TGE::Con::printf("Loaded TTStates: %s", ttstates.c_str());
			powerupstates = read_list_int(&m);
			//TGE::Con::printf("Loaded PowStates: %s", powerupstates.c_str());
		}
		std::string gamestate = m.readString();
		//TGE::Con::printf("Loaded Gametate: %s", gamestate.c_str());
		if (version >= 7)
			lmstates = read_vector<int>(&m);
		else
			lmstates = read_list_int(&m);
		//TGE::Con::printf("Loaded LMStates: %s", lmstates.c_str());
		int nextStateTime = m.readInt32();

		if (version >= 7)
		{
			activepowstates = read_vector<int>(&m);
		}
		else
			activepowstates = read_list_powerupstates(&m);
		//TGE::Con::printf("Loaded ActivePowStates: %s", activepowstates.c_str());
		std::string gravitydir = m.readString();
		//TGE::Con::printf("Loaded GDirStates: %s", gravitydir.c_str());
		std::vector<int> trapdoordirs; 
		std::vector<int> trapdoorclose;
		std::vector<int> trapdooropen; 
		std::vector<float>trapdoorpos; 
		if (version >= 5 && version < 7)
		{
			trapdoordirs = read_list_int(&m);
			trapdooropen = read_list_int(&m);
			trapdoorclose = read_list_int(&m);
		}
		if (version >= 6 && version < 7)
		{
			trapdoorpos = read_list_float(&m);
		}
		if (version >= 7)
		{
			trapdoordirs = read_vector<int>(&m);
			trapdooropen = read_vector<int>(&m);
			trapdoorclose = read_vector<int>(&m);
			trapdoorpos = read_vector<float>(&m);

		}
#ifdef MBP
		TeleportState ts;
		if (version >= 8)
		{
			ts.teleportDelay = m.readInt32();
			ts.destination = m.readString();
			if (version >= 9)
				ts.teleportCounter = m.readInt32();
		}
		bool eggstate = false;
		if (version >= 12)
			eggstate = m.readBool();

#endif // MBP

		std::vector<RewindableState<int>> rewindableIntStates;
		std::vector<RewindableState<float>> rewindableFloatStates;
		std::vector<RewindableState<bool>> rewindableBoolStates;
		std::vector<RewindableState<std::string>> rewindableStringStates;

		std::vector<RewindableState<int>> rewindableSOIntStates;
		std::vector<RewindableState<float>> rewindableSOFloatStates;
		std::vector<RewindableState<bool>> rewindableSOBoolStates;
		std::vector<RewindableState<std::string>> rewindableSOStringStates;

		if (version == 10)
		{
			m.readInt32();
			m.readInt32();
		}
		if (version >= 11)
		{
			rewindableIntStates = read_vector_rewindable<int>(&m);
			rewindableFloatStates = read_vector_rewindable<float>(&m);
			rewindableBoolStates = read_vector_rewindable<bool>(&m);
			rewindableStringStates = read_vector_rewindable<std::string>(&m);

			rewindableSOIntStates = read_vector_rewindable<int>(&m);
			rewindableSOFloatStates = read_vector_rewindable<float>(&m);
			rewindableSOBoolStates = read_vector_rewindable<bool>(&m);
			rewindableSOStringStates = read_vector_rewindable<std::string>(&m);
		}

		if (deltaMs < 0)
		{
			framecount--;
			continue;
		}
		if (isGhost)
		{
			if (hasMs(ms))
			{
				if (m.tell() >= m.length()) break;
				continue;
			}
		}
		if (isGhost) //Don't add those stopped time frames
		{
			if (Frames.size() != 0)
				if (Frames.back().ms == ms)
					continue;
		}

		Frame frame = Frame(ms, deltaMs, position, velocity, spin, pow, timebonus, mpstates, gemcount, gemstates, ttstates, powerupstates, gamestate, lmstates, nextStateTime, activepowstates, gravitydir, trapdoordirs, trapdooropen, trapdoorclose, trapdoorpos
#ifdef MBP
			, ts //Too hackery
			, CheckpointState() //Yeah uh we arent saving checkpoint states
			, eggstate
#endif // MBP
		);

		frame.rewindableIntStates = rewindableIntStates;
		frame.rewindableFloatStates = rewindableFloatStates;
		frame.rewindableBoolStates = rewindableBoolStates;
		frame.rewindableStringStates = rewindableStringStates;
		frame.rewindableSOIntStates = rewindableSOIntStates;
		frame.rewindableSOFloatStates = rewindableSOFloatStates;
		frame.rewindableSOBoolStates = rewindableSOBoolStates;
		frame.rewindableSOStringStates = rewindableSOStringStates;

		Frames.push_back(frame);
		if (m.tell() >= m.length()) break;
	}


	TGE::Con::printf("Loaded replay %s, %d Frames", replayPath.c_str(), framecount);
	setFrameElapsedTimes();
	std::reverse(Frames.begin(), Frames.end());
	setUpFrameStreaming();
	DebugPop("Leaving RewindManager::load");
	return replayMission.c_str();
}

ReplayInfo RewindManager::analyze(std::string path)
{
	DebugPush("Entering RewindManager::analyzze(%s)", path.c_str());
	Frames.clear();
	this->totalTime = 0;

	clearSaveStates();

	FILE* f;

	ReplayInfo info;


#ifdef  __APPLE__
	std::replace(path.begin(), path.end(), '\\', '/');
#endif //  __APPLE__

	f = fopen(path.c_str(), "rb");

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);
	uint8_t* buffer = new uint8_t[size + 1];
	fread(buffer, sizeof(uint8_t), size, f);
	fclose(f);

	MemoryStream m;
	m.createFromBuffer(buffer, size);
	fclose(f);

	info.replayPath = path;

	char version = m.readChar();
	info.version = version;

	unsigned long uncompressedSize = 52428800; //max uncompressed data size - 50mb, bad idea but replays prob wont go over this
	if (version >= 3)
		uncompressedSize = m.readInt32();

	info.replayMission = std::string("[null]");

	if (version >= 2)
	{
		TGE::Con::printf("Uncompressing Replay");

		uint8_t* buf = &m.getBuffer()[m.tell()];
		uint64_t size = m.length();
		size -= 1; // one byte used by version
		if (version >= 3)
			size -= sizeof(unsigned long);

		uint8_t* uncompressed = new uint8_t[uncompressedSize + 1];

		uncompress((Bytef*)uncompressed, &uncompressedSize, (Bytef*)buf, size);

		m.createFromBuffer(uncompressed, uncompressedSize);


	}
	int framecount = m.readInt32();
	info.frameCount = framecount;
	if (version >= 4)
		info.replayMission = m.readString();
	if (version >= 10)
	{
		std::string replaygame = m.readString();
	}


	int i;
	info.elapsedTime = 0;
	info.time = 0;
	for (i = 0; i < framecount; i++)
	{
		int ms, deltaMs;
		ms = m.readInt32();
		deltaMs = m.readInt32();

		if (i == 0)
			info.time = ms;
		info.elapsedTime += deltaMs;

		//TGE::Con::printf("Loaded Time: %d %d", ms,deltaMs);
		m.readDouble();
		m.readDouble();
		m.readDouble();
		m.readDouble();
		m.readDouble();
		m.readDouble();
		m.readDouble();
		m.readDouble();
		m.readDouble();
		m.readInt32();
		m.readInt32();

		read_list_mpstates(&m);
		m.readInt32();

		if (version >= 7)
		{
			read_vector<int>(&m);
			read_vector<int>(&m);
			read_vector<int>(&m);
		}
		else
		{
			read_list_int(&m);
			read_list_int(&m);
			read_list_int(&m);
		}
		m.readString();

		if (version >= 7)
			read_vector<int>(&m);
		else
			read_list_int(&m);
		m.readInt32();

		if (version >= 7)
		{
			read_vector<int>(&m);
		}
		else
			read_list_powerupstates(&m);
		m.readString();
		if (version >= 5 && version < 7)
		{
			read_list_int(&m);
			read_list_int(&m);
			read_list_int(&m);
		}
		if (version >= 6 && version < 7)
		{
			read_list_float(&m);
		}
		if (version >= 7)
		{
			read_vector<int>(&m);
			read_vector<int>(&m);
			read_vector<int>(&m);
			read_vector<float>(&m);
		}
#ifdef MBP
		TeleportState ts;
		if (version >= 8)
		{
			m.readInt32();
			m.readString();
			if (version >= 9)
				m.readInt32();
		}
		if (version >= 12)
			m.readBool();

#endif // MBP


		if (version == 10)
		{
			m.readInt32();
			m.readInt32();
		}
		if (version >= 11)
		{
			read_vector_rewindable<int>(&m);
			read_vector_rewindable<float>(&m);
			read_vector_rewindable<bool>(&m);
			read_vector_rewindable<std::string>(&m);

			read_vector_rewindable<int>(&m);
			read_vector_rewindable<float>(&m);
			read_vector_rewindable<bool>(&m);
			read_vector_rewindable<std::string>(&m);
		}

		if (deltaMs < 0)
		{
			framecount--;
			continue;
		}

		if (m.tell() >= m.length()) break;
		}


	DebugPop("Leaving RewindManager::analyze");
	return info;
}

void RewindManager::clear(bool write, Worker* worker)
{
	DebugPush("Entering RewindManager::clear(%d)", write);
	if (write)
	{
		worker->addTask([this]() {
			this->save(replayPath);		
			this->Frames.clear();
			this->pathedInteriors = NULL;
			this->totalTime = 0;
			DebugPop("Leaving RewindManager::clear");
		});
		
	}
	else
	{
		this->Frames.clear();
		this->pathedInteriors = NULL;
		this->totalTime = 0;
		DebugPop("Leaving RewindManager::clear");
	}
}

template<typename T>
RewindableState<T> RewindManager::InterpolateRewindableState(RewindableState<T> one, RewindableState<T> two, float ratio, float delta)
{
	DebugPush("Entering RewindManager::InterpolateRewindableState(%s,%f,%f)", one.bindingnamespace.c_str(), "", ratio, delta);
	RewindableBindingBase* binding = NULL;
	for (int i = 0; i < this->rewindableBindings.size(); i++)
	{
		if (rewindableBindings[i]->BindingNamespace == one.bindingnamespace)
		{
			binding = rewindableBindings[i];
			break;
		}
	}

	if (binding == NULL)
	{
		TGE::Con::errorf("RewindManager::InterpolateRewindableState CANNOT INTERPOLATE FOR NAMESPACE %s", one.bindingnamespace.c_str());
	}

	int type = binding->getStorageType();

	RewindableState<T> rs = RewindableState<T>(binding->BindingNamespace);

	RewindableBinding<T>* b = static_cast<RewindableBinding<T>*>(binding);
	rs.value = b->interpolateState(one.value, two.value, ratio, delta);
	DebugPop("Leaving RewindManager::InterpolateRewindableState()");
	return rs;

}

Frame RewindManager::interpolateFrame(Frame one,Frame two,float ratio,float delta)
{

	DebugPush("Entering RewindManager::interpolateFrame(one,two,%f,%f)", ratio,delta);
	Frame f = Frame();
	f.deltaMs = delta;

	if (one.timebonus > 0 && two.timebonus > 0)
		f.ms = fmin(one.ms,two.ms); //Stop time while rewinding
	else
	{
		f.ms = mLerp(one.ms, two.ms, ratio);
	}


#ifdef MBP
	bool isTeleporting = CheckForTeleport(one.teleportState, two.teleportState) || CheckForRespawn(one.checkpointState,two.checkpointState);
	if (isTeleporting)
	{
		f.position = (ratio > 0.5) ? two.position : one.position;
		f.velocity = (ratio > 0.5) ? two.velocity : one.velocity;
		f.spin = (ratio > 0.5) ? two.spin : one.spin;
	}
	else
	{
#endif // MBP
		f.position = Point3D(mLerp(one.position.x, two.position.x, ratio), mLerp(one.position.y, two.position.y, ratio), mLerp(one.position.z, two.position.z, ratio));
		f.velocity = Point3D(mLerp(one.velocity.x, two.velocity.x, ratio), mLerp(one.velocity.y, two.velocity.y, ratio), mLerp(one.velocity.z, two.velocity.z, ratio));
		f.spin = Point3D(mLerp(one.spin.x, two.spin.x, ratio), mLerp(one.spin.y, two.spin.y, ratio), mLerp(one.spin.z, two.spin.z, ratio));
#ifdef MBP
	}
#endif
	f.powerup = two.powerup;

	bool isTTpickedup = !CompareListEquality(one.ttstates, two.ttstates);

	if (isTTpickedup)
		f.timebonus = two.timebonus;
	else
		f.timebonus = mLerp(one.timebonus, two.timebonus, ratio);


	if (pathedInteriors != NULL)
		f.mpstates = InterpolateMPStates(one.mpstates, two.mpstates, *pathedInteriors, ratio,delta);
	else
		f.mpstates = std::vector<MPState>();
	f.gemcount = two.gemcount;
	f.gemstates = two.gemstates;
	f.ttstates = two.ttstates;
	f.powerupstates = InterpolateList<int>(one.powerupstates, two.powerupstates, ratio);
	f.gamestate = two.gamestate;
	f.lmstates = InterpolateList<int>(one.lmstates, two.lmstates, ratio);
	f.nextstatetime = mFloor(InterpolateNextStateTimer(one, two, ratio));//mLerp(one.nextstatetime, two.nextstatetime, ratio);
	f.activepowstates = InterpolateList<int>(one.activepowstates, two.activepowstates, ratio);
	f.gravityDir = two.gravityDir;
	f.trapdoordirs = two.trapdoordirs;
	f.trapdooropen = InterpolateList<int>(one.trapdooropen, two.trapdooropen, ratio);
	f.trapdoorclose = InterpolateList<int>(one.trapdoorclose, two.trapdoorclose, ratio);
	f.trapdoorpos = InterpolateList<float>(one.trapdoorpos, two.trapdoorpos, ratio);
	f.elapsedTime = mLerp(one.elapsedTime, two.elapsedTime, ratio);
#ifdef  MBP
	f.teleportState = InterpolateTeleportState(one.teleportState, two.teleportState, ratio);
	f.checkpointState = (ratio > 0.5) ? two.checkpointState : one.checkpointState;
	f.eggstate = (ratio > 0.5) ? two.eggstate : one.eggstate;
#endif //  MBP

#ifdef WIN32
#define InterpolateRewindable(type) for (int i = 0; i < one.rewindable##type##States.size(); i++) \
	f.rewindable##type##States.push_back(InterpolateRewindableState(one.rewindable##type##States[i], two.rewindable##type##States[i], ratio, delta));

	// Epic macro usage

	InterpolateRewindable(Int);
	InterpolateRewindable(Float);
	InterpolateRewindable(Bool);
	InterpolateRewindable(String);

	InterpolateRewindable(SOInt);
	InterpolateRewindable(SOFloat);
	InterpolateRewindable(SOBool);
	InterpolateRewindable(SOString);

#undef InterpolateRewindable(type)
#endif
#ifdef __APPLE__
	for (int i = 0; i < one.rewindableIntStates.size(); i++)
		f.rewindableIntStates.push_back(InterpolateRewindableState(one.rewindableIntStates[i], two.rewindableIntStates[i], ratio, delta));
	for (int i = 0; i < one.rewindableFloatStates.size(); i++)
		f.rewindableFloatStates.push_back(InterpolateRewindableState(one.rewindableFloatStates[i], two.rewindableFloatStates[i], ratio, delta));
	for (int i = 0; i < one.rewindableBoolStates.size(); i++)
		f.rewindableBoolStates.push_back(InterpolateRewindableState(one.rewindableBoolStates[i], two.rewindableBoolStates[i], ratio, delta));
	for (int i = 0; i < one.rewindableStringStates.size(); i++)
		f.rewindableStringStates.push_back(InterpolateRewindableState(one.rewindableStringStates[i], two.rewindableStringStates[i], ratio, delta));

	for (int i = 0; i < one.rewindableSOIntStates.size(); i++)
		f.rewindableSOIntStates.push_back(InterpolateRewindableState(one.rewindableSOIntStates[i], two.rewindableSOIntStates[i], ratio, delta));
	for (int i = 0; i < one.rewindableSOFloatStates.size(); i++)
		f.rewindableSOFloatStates.push_back(InterpolateRewindableState(one.rewindableSOFloatStates[i], two.rewindableSOFloatStates[i], ratio, delta));
	for (int i = 0; i < one.rewindableSOBoolStates.size(); i++)
		f.rewindableSOBoolStates.push_back(InterpolateRewindableState(one.rewindableSOBoolStates[i], two.rewindableSOBoolStates[i], ratio, delta));
	for (int i = 0; i < one.rewindableSOStringStates.size(); i++)
		f.rewindableSOStringStates.push_back(InterpolateRewindableState(one.rewindableSOStringStates[i], two.rewindableSOStringStates[i], ratio, delta));
#endif
	DebugPop("Leaving RewindManager::interpolateFrame");
	return f;

}

Frame* RewindManager::getRealtimeFrameAtMs(float ms)
{
	DebugPush("Entering RewindManager::getRealtimeFrameAtMs(%f)", ms);
	//basically do a binary search

	if (ms < Frames[0].ms)
		return new Frame(Frames[0]);
	if (ms > Frames.back().ms)
		return new Frame(Frames.back());

	int lo = 0, hi = Frames.size() - 1;
	int m;

	int index0 = -1;
	int index1 = -2;

	while (lo <= hi)
	{
		m = (lo + hi) / 2;

		if (Frames[m].ms < ms)
			lo = m + 1;
		else if (Frames[m].ms > ms)
			hi = m - 1;
		else
		{
			index0 = index1 = m;
			break;
		}
	}

	if (index0 == index1) //We did find the frame, no need to interpolate
		return new Frame(Frames[index0]);

	if (index0 == -1 && index1 == -2)	//We didnt find the exact frame, need to interpolate
	{
		index0 = lo;
		index1 = hi;
	}


	if (index0 > index1) //Sort the indexes to ascending order
	{
		int temp = index0;
		index0 = index1;
		index1 = temp;
	}

	double ratio = (double)(ms - Frames[index0].ms) / (double)(Frames[index1].ms - Frames[index0].ms);
	DebugPop("Leaving RewindManager::getRealtimeFrameAtMs");
	return new Frame(interpolateFrame(Frames[index0], Frames[index1], ratio, ms));
}

Frame* RewindManager::getFrameAtElapsedMs(float ms)
{
	DebugPush("Entering RewindManager::getFrameAtElapsedMs(%f)",ms);
	// binary search version cause apparently the old algorithm consistently breaks while
	// scrubbing very specific replays, i have yet to find why it doesnt work for those and what causes those replays in the first place
	// 8/1/2021: bruh fuck that, lets all forget the garbage that was the old algorithm
	if (ms < Frames[0].elapsedTime)
		return new Frame(Frames[0]);
	if (ms > Frames.back().elapsedTime)
		return NULL;

	int lo = 0, hi = Frames.size() - 1;
	int m;

	int index0 = -1;
	int index1 = -2;

	while (lo <= hi)
	{
		m = (lo + hi) / 2;

		if (Frames[m].elapsedTime < ms)
			lo = m + 1;
		else if (Frames[m].elapsedTime > ms)
			hi = m - 1;
		else
		{
			index0 = index1 = m;
			break;
		}
	}

	if (index0 == index1) //We did find the frame, no need to interpolate
		return new Frame(Frames[index0]);

	if (index0 == -1 && index1 == -2)	//We didnt find the exact frame, need to interpolate
	{
		index0 = lo;
		index1 = hi;
	}


	if (index0 > index1) //Sort the indexes to ascending order
	{
		int temp = index0;
		index0 = index1;
		index1 = temp;
	}

	double ratio = (double)(ms - Frames[index0].elapsedTime) / (double)(Frames[index1].elapsedTime - Frames[index0].elapsedTime);
	DebugPop("Leaving RewindManager::getFrameAtElapsedMs");
	return new Frame(interpolateFrame(Frames[index0], Frames[index1], ratio, ms));
}

Frame* RewindManager::getFrameAtMs(float ms,int index = -1,bool useElapsed)
{
	DebugPush("Entering RewindManager::getFrameAtMs(%f,%d,%d)",ms,index,useElapsed);
	if (ms < 0) return NULL;

	if (index == -1) index = Frames.size() - 1;
	if (index >= Frames.size()) index = Frames.size() - 1;

	if (useElapsed)
	{
		if (ms < Frames.back().elapsedTime)
		{
			DebugPop("Leaving RewindManager::getFrameAtMs");
			return new Frame(Frames.back());
		}
	}
	else
	{
		if (streamTimePosition > ms)
		{
			currentIndex += ceilf((streamTimePosition - ms) / averageDelta) + 3;

			if (currentIndex >= Frames.size()) currentIndex = Frames.size() - 1;

			index = currentIndex;
		}

		if (ms < Frames.back().ms)
		{
			DebugPop("Leaving RewindManager::getFrameAtMs");
			return new Frame(Frames.back());
		}
	}

	for (int i = index;i >= 0; i--) 
	{
		Frame f = Frames[i];
		if (useElapsed)
		{
			if (f.elapsedTime > ms)
			{
				double ratio = (float)((f.deltaMs - (f.elapsedTime - ms))) / (float)f.deltaMs;
				DebugPop("Leaving RewindManager::getFrameAtMs");
				return new Frame(interpolateFrame(Frames[i + 1], f, ratio, ms));
			}
		}
		else
		{
			if (f.ms > ms)
			{
				double ratio = (float)((f.deltaMs - (f.ms - ms))) / (float)f.deltaMs;
				currentIndex = i;
				streamTimePosition = ms;
				DebugPop("Leaving RewindManager::getFrameAtMs");
				return new Frame(interpolateFrame(Frames[i + 1], f, ratio, ms));
			}
		}
	}
	DebugPop("Leaving RewindManager::getFrameAtMs");
	return NULL;

}

Frame* RewindManager::getNextRewindFrame(float delta)
{
	DebugPush("Entering RewindManager::getNextRewindFrame(%f)", delta);
	if (delta < 0) return NULL;

	if (Frames.size() == 0) return NULL;

	if (Frames.size() >= 2)
	{
		if (delta < Frames.back().deltaMs)
		{
			Frame first = Frames.back();
			Frame second = Frames[Frames.size() - 2];
			Frames.pop_back();

			Frame interpolated = interpolateFrame(first, second, ((float)delta) / ((float)first.deltaMs), delta);

			Frames.push_back(interpolated);

			DebugPop("Leaving RewindManager::getNextRewindFrame");
			return new Frame(interpolated);
		}
		else
		{
			Frame first = Frames.back();

			int deltaAccumulator = 0;
			Frame midframe = Frames.back();

			bool outOfFrames = 0;

			while (deltaAccumulator < delta)
			{
				if (Frames.size() == 0)
				{
					outOfFrames = 1;
					break;
				}
				midframe = Frames.back();
				Frames.pop_back();
				deltaAccumulator += midframe.deltaMs;
			}
			if (!outOfFrames)
			{
				//Frames.pop_back();

				Frame lastframe = Frames.size() == 0 ? midframe : Frames.back();

				Frame interpolated = interpolateFrame(first, lastframe, ((float)delta) / ((float)deltaAccumulator), deltaAccumulator - delta);

				Frames.push_back(interpolated);
				DebugPop("Leaving RewindManager::getNextRewindFrame");
				return new Frame(interpolated);
			}
			else
			{
				Frame interpolated = interpolateFrame(first, midframe, ((float)delta) / ((float)deltaAccumulator), deltaAccumulator - delta);

				Frames.push_back(interpolated);
				DebugPop("Leaving RewindManager::getNextRewindFrame");
				return new Frame(interpolated);
			}

		}
	}
	else
	{
		Frame ret = Frames.back();
		Frames.pop_back();
		DebugPop("Leaving RewindManager::getNextRewindFrame");
		return new Frame(ret);
	}
	DebugPop("Leaving RewindManager::getNextRewindFrame");
}

Frame* RewindManager::getNextFrame(float delta)
{
	DebugPush("Entering RewindManager::getNextFrame(%f)", delta);
	streamTimePosition += delta;

	if (streamTimePosition < 0) streamTimePosition = 0;

	int timepos = streamTimePosition;

	Frame* testF = getFrameAtElapsedMs(streamTimePosition); 

	if (testF == NULL) return NULL;

	Frame* f = new Frame(*testF);
	
	DebugPop("Leaving RewindManager::getNextFrame");
	return f;
}

Frame* RewindManager::getNextNonElapsedFrame(float delta)
{
	DebugPush("Entering RewindManager::getNextNonElpasedFrame(%f)", delta);
	streamTimePosition += delta;

	if (streamTimePosition < 0) streamTimePosition = 0;

	int timepos = streamTimePosition;

	if (delta < 0) currentIndex += (ceilf((float)delta / averageDelta) + 3); //Just to be safe

	Frame* testF = getFrameAtMs(streamTimePosition, currentIndex + 3,false); //To be safe, i used 3, can be any positive arbitrary number

	if (testF == NULL) return NULL;

	Frame* f = new Frame(*testF);

	if (currentIndex >= Frames.size())
	{
		currentIndex = Frames.size() - 1;
	}

	for (int i = currentIndex; i >= 0; i--)
	{
		if (Frames[i].ms < streamTimePosition)
		{
			currentIndex--;
		}
		else
		{
			break;
		}
	}

	DebugPop("Leaving RewindManager::getNextNonElapsedFrame");
	return f;
}

int RewindManager::getSavedStateCount()
{
	DebugPush("Entering and Leaving RewindManager::getSavedStateCount()");
	return SaveStates.size();
}

void RewindManager::clearSaveStates()
{
	DebugPush("Entering RewindManager::clearSaveStates");
	SaveStates.clear();
	DebugPop("Leaving RewindManager::clearSaveStates");
}

void RewindManager::saveState()
{
	DebugPush("Entering RewindManager::saveState");
	SaveStates.push_back(Frames);
	DebugPop("Leaving RewindManager::saveState");
}

void RewindManager::loadState(int index)
{
	DebugPush("Entering RewindManager::loadState");
	Frames = SaveStates[index];
	DebugPop("Leaving RewindManager::loadState");
}

void RewindManager::spliceReplayFromMs(float ms)
{
	DebugPush("Entering RewindManager::spliceReplayFromMs");
	std::vector<Frame> newFrames;
	Frame* atMs = this->getFrameAtElapsedMs(ms);
	for (auto& frame : Frames)
	{
		if (frame.elapsedTime < ms)
			newFrames.push_back(frame);
		else
			break;
	}
	if (atMs != NULL)
		newFrames.push_back(*atMs);
	Frames = newFrames;
	DebugPop("Leaving RewindManager::spliceReplayFromMs");
}