// TGE.h: Interface to core TGE functions.
// Usable both from plugins and PluginLoader.dll.

#ifndef TORQUELIB_TGE_H
#define TORQUELIB_TGE_H

#include <cstdarg>

#include "platform/platform.h"
#include "math/mMath.h"
#include "util/tVector.h"

#ifdef _DEBUG
#define DEBUG_PRINTF(fmt, ...) TGE::Con::printf(fmt, __VA_ARGS__)
#define DEBUG_WARNF(fmt, ...)  TGE::Con::warnf(fmt, __VA_ARGS__)
#define DEBUG_ERRORF(fmt, ...) TGE::Con::errorf(fmt, __VA_ARGS__)
#else
#define DEBUG_PRINTF(fmt, ...)
#define DEBUG_WARNF(fmt, ...)
#define DEBUG_ERRORF(fmt, ...)
#endif

#include "linux/InterfaceMacros-linux.h"
#include "osx/Addresses-osx.h"

namespace TGE
{
	// Class prototypes
	class SimObject;
	class BaseMatInstance;
	class GameConnection;
	class Camera;
	class ResourceObject;
	class NetConnection;
	class BitStream;
	struct Move
	{
		// packed storage rep, set in clamp
		S32 px, py, pz;
		U32 pyaw, ppitch, proll;
		F32 x, y, z;          // float -1 to 1
		F32 yaw, pitch, roll; // 0-2PI
		U32 id;               // sync'd between server & client - debugging tool.
		U32 sendCount;

		bool freeLook;
		bool trigger[4];

	};
}

// TGE callback types
typedef const char* (*StringCallback)(TGE::SimObject *obj, S32 argc, const char *argv[]);
typedef S32         (*IntCallback)   (TGE::SimObject *obj, S32 argc, const char *argv[]);
typedef F32         (*FloatCallback) (TGE::SimObject *obj, S32 argc, const char *argv[]);
typedef void        (*VoidCallback)  (TGE::SimObject *obj, S32 argc, const char *argv[]);
typedef bool        (*BoolCallback)  (TGE::SimObject *obj, S32 argc, const char *argv[]);

typedef U32 SimObjectId;
typedef S32 NetSocket;

namespace TGE
{
	namespace Namespace {
		class Namespace;
	}

	class AbstractClassRep;

	class ConsoleObject
	{
	public:
		VTABLE(0);
		
		VIRTFNSIMP(AbstractClassRep*, getClassRep, 0);
		VIRTDTOR(~ConsoleObject, TGEVIRT_CONSOLEOBJECT_DESTRUCTOR);
	};

	class SimObject: public ConsoleObject
	{
	public:
		GETTERFNSIMP(SimObjectId, getId, TGEOFF_SIMOBJECT_ID);
		MEMBERFNSIMP(const char*, getIdString, TGEADDR_SIMOBJECT_GETIDSTRING);
		GETTERFNSIMP(const char *, getName, 0x4);
		MEMBERFN(void, setHidden, (bool hidden), (hidden), TGEADDR_SIMOBJECT_SETHIDDEN);
		MEMBERFN(const char*, getDataField, (const char* slotName, const char* array), (slotName, array), 0x2B890);
		MEMBERFN(void, setDataField, (const char* slotName, const char* array, const char* value), (slotName, array, value), 0x29A20);

		UNDEFVIRT(processArguments);
		UNDEFVIRT(onAdd);
		UNDEFVIRT(onRemove);
		UNDEFVIRT(onGroupAdd);
		UNDEFVIRT(onGroupRemove);
		UNDEFVIRT(onNameChange);
		UNDEFVIRT(onStaticModified);
		UNDEFVIRT(inspectPreApply);
		UNDEFVIRT(inspectPostApply);
		UNDEFVIRT(onVideoKill);
		UNDEFVIRT(onVideoResurrect);
		UNDEFVIRT(onDeleteNotify);
		UNDEFVIRT(onEditorEnable);
		UNDEFVIRT(onEditorDisable);
		VIRTFN(const char *, getEditorClassName, (const char* name), (name), 17);
		UNDEFVIRT(findObject);
		UNDEFVIRT(write);
		UNDEFVIRT(registerLights);
	};

	class SimObjectList: public VectorPtr<SimObject*>
	{
	public:
		SimObject *at(S32 index) const
		{
			if (index >= 0 && index < size())
				return (*this)[index];
			return NULL;
		}
	};

	class SimSet: public SimObject
	{
		char unknown[0x2c];
	public:
		GETTERFNSIMP(SimObjectList*, getSimObjectList, 48);
		SimObjectList objectList;

		GETTERFNSIMP(int, getCount, 0x30);
		UNDEFVIRT(onRemove);
		UNDEFVIRT(onDeleteNotify);
		UNDEFVIRT(addObject);
		UNDEFVIRT(removeObject);
		UNDEFVIRT(pushObject);
		UNDEFVIRT(popObject);
		UNDEFVIRT(write);
		UNDEFVIRT(findObject);
	};

	class SimNameDictionary
	{
	};

	class SimGroup: public SimSet
	{
	};

	class GuiControl: public SimGroup
	{
	public:
		GETTERFNSIMP(RectI, getBounds, TGEOFF_GUICONTROL_BOUNDS);
		GETTERFNSIMP(Point2I, getPosition, TGEOFF_GUICONTROL_POSITION);
		GETTERFNSIMP(Point2I, getExtent, TGEOFF_GUICONTROL_EXTENT);
	};

	class GuiCanvas: public GuiControl
	{
	};

	class NetObject: public SimObject
	{
	public:
		UNDEFVIRT(getUpdatePriority);
		VIRTFN(U32, packUpdate, (NetConnection *conn, U32 mask, BitStream *stream), (conn, mask, stream), TGEVIRT_NETOBJECT_PACKUPDATE);
		VIRTFN(void, unpackUpdate, (NetConnection *conn, BitStream *stream), (conn, stream), TGEVIRT_NETOBJECT_UNPACKUPDATE);
		UNDEFVIRT(onCameraScopeQuery);

        GETTERFNSIMP(U32, getNetFlags, 0x40);
        SETTERFN(U32, setNetFlags, 0x40);

		MEMBERFN(void, setMaskBits, (U32 bits), (bits), 0x196B40);

        inline bool isServerObject() { return (getNetFlags() & 2) == 0; }
        inline bool isClientObject() { return (getNetFlags() & 2) != 0; }
    };

	class SceneObject: public NetObject
	{
	public:
		VIRTFNSIMP(void, disableCollision, TGEVIRT_SCENEOBJECT_DISABLECOLLISION);
		VIRTFNSIMP(void, enableCollision, TGEVIRT_SCENEOBJECT_ENABLECOLLISION);
		UNDEFVIRT(isDisplacable);
		UNDEFVIRT(getMomentum);
		UNDEFVIRT(setMomentum);
		UNDEFVIRT(getMass);
		UNDEFVIRT(displaceObject);
		VIRTFN(void, setTransformVirt, (const MatrixF &transform), (transform), TGEVIRT_SCENEOBJECT_SETTRANSFORM);
		UNDEFVIRT(setScale);
		UNDEFVIRT(setRenderTransform);
		UNDEFVIRT(buildConvex);
		UNDEFVIRT(buildPolyList);
		UNDEFVIRT(buildCollisionBSP);
		UNDEFVIRT(castRay);
		UNDEFVIRT(collideBox);
		UNDEFVIRT(getOverlappingZones);
		UNDEFVIRT(getPointZone);
		UNDEFVIRT(renderShadowVolumes);
		UNDEFVIRT(renderObject);
		UNDEFVIRT(prepRenderImage);
		UNDEFVIRT(scopeObject);
		UNDEFVIRT(getMaterialProperty);
		UNDEFVIRT(onSceneAdd);
		UNDEFVIRT(onSceneRemove);
		UNDEFVIRT(transformModelview);
		UNDEFVIRT(transformPosition);
		UNDEFVIRT(computeNewFrustum);
		UNDEFVIRT(openPortal);
		UNDEFVIRT(closePortal);
		UNDEFVIRT(getWSPortalPlane);
		UNDEFVIRT(installLights);
		UNDEFVIRT(uninstallLights);
		UNDEFVIRT(getLightingAmbientColor);

		MEMBERFN(void, setTransform, (const MatrixF &transform), (transform), 0x01909A0);

		SETTERFN(MatrixF, setTransformMember, TGEOFF_SCENEOBJECT_TRANSFORM);

		GETTERFN(const MatrixF &, MatrixF, getTransform, TGEOFF_SCENEOBJECT_TRANSFORM);
		GETTERFN(const Box3F &, Box3F, getWorldBox, TGEOFF_SCENEOBJECT_WORLDBOX);
	};

	class SimDataBlock : public SimObject
	{

	};

	class GameBaseData : public SimDataBlock
	{

	};

	class GameBase : public SceneObject
	{
	public:
		GETTERFNSIMP(GameConnection*, getControllingClient, TGEOFF_GAMEBASE_CONTROLLINGCLIENT);
		GETTERFNSIMP(GameBaseData *, getDataBlock, 0x248);

		UNDEFVIRT(onNewDataBlock);
		UNDEFVIRT(processTick);
		UNDEFVIRT(interpolateTick);
		UNDEFVIRT(advanceTime);
		UNDEFVIRT(advancePhysics);
		UNDEFVIRT(getVelocity);
		UNDEFVIRT(getForce);
		UNDEFVIRT(writePacketData);
		UNDEFVIRT(readPacketData);
		UNDEFVIRT(getPacketDataChecksum);
	};

	class ShapeBaseImageData;

	class TSShapeInstance;

	class TSThread
	{
	public:
		GETTERFNSIMP(float, getPos, 0xC);
		SETTERFN(float, setPos, 0xC);
	};

	struct Thread {
		enum State {
			Play, Stop, Pause, FromMiddle
		};
		TSThread* thread;
		U32 state;


		S32 sequence;
		U32 sound;
		bool atEnd;
		bool forward;
	};

	class TSShapeInstance
	{
	public:
		MEMBERFN(void, setPos, (TSThread * thread, F32 pos), (thread, pos), 0x1B27F0);
		MEMBERFN(void, setTimeScale, (TSThread * thread, F32 t), (thread, t), 0x1B0AE0);
	};

	class ShapeBase : public GameBase
	{
	public:
		UNDEFVIRT(setImage);
		UNDEFVIRT(onImageRecoil);
		UNDEFVIRT(ejectShellCasing);
		UNDEFVIRT(updateDamageLevel);
		UNDEFVIRT(updateDamageState);
		UNDEFVIRT(blowUp);
		UNDEFVIRT(onMount);
		UNDEFVIRT(onUnmount);
		UNDEFVIRT(onImpact_SceneObject_Point3F);
		UNDEFVIRT(onImpact_Point3F);
		UNDEFVIRT(controlPrePacketSend);
		UNDEFVIRT(setEnergyLevel);
		UNDEFVIRT(mountObject);
		UNDEFVIRT(mountImage);
		UNDEFVIRT(unmountImage);
		UNDEFVIRT(getMuzzleVector);
		UNDEFVIRT(getCameraParameters);
		VIRTFN(void, getCameraTransform, (F32 *pos, MatrixF *mat), (pos, mat), TGEVIRT_SHAPEBASE_GETCAMERATRANSFORM);
		UNDEFVIRT(getEyeTransform);
		UNDEFVIRT(getRetractionTransform);
		UNDEFVIRT(getMountTransform);
		UNDEFVIRT(getMuzzleTransform);
		UNDEFVIRT(getImageTransform_uint_MatrixF);
		UNDEFVIRT(getImageTransform_uint_int_MatrixF);
		UNDEFVIRT(getImageTransform_uint_constchar_MatrixF);
		UNDEFVIRT(getRenderRetractionTransform);
		UNDEFVIRT(getRenderMountTransform);
		UNDEFVIRT(getRenderMuzzleTransform);
		UNDEFVIRT(getRenderImageTransform_uint_MatrixF);
		UNDEFVIRT(getRenderImageTransform_uint_int_MatrixF);
		UNDEFVIRT(getRenderImageTransform_uint_constchar_MatrixF);
		UNDEFVIRT(getRenderMuzzleVector);
		UNDEFVIRT(getRenderMuzzlePoint);
		UNDEFVIRT(getRenderEyeTransform);
		UNDEFVIRT(getDamageFlash);
		UNDEFVIRT(setDamageFlash);
		UNDEFVIRT(getWhiteOut);
		UNDEFVIRT(setWhiteOut);
		UNDEFVIRT(getInvincibleEffect);
		UNDEFVIRT(setupInvincibleEffect);
		UNDEFVIRT(updateInvincibleEffect);
		UNDEFVIRT(setVelocity);
		UNDEFVIRT(applyImpulse);
		UNDEFVIRT(setControllingClient);
		UNDEFVIRT(setControllingObject);
		UNDEFVIRT(getControlObject);
		UNDEFVIRT(setControlObject);
		UNDEFVIRT(getCameraFov);
		UNDEFVIRT(getDefaultCameraFov);
		UNDEFVIRT(setCameraFov);
		UNDEFVIRT(isValidCameraFov);
		UNDEFVIRT(renderMountedImage);
		UNDEFVIRT(renderImage);
		UNDEFVIRT(calcClassRenderData);
		UNDEFVIRT(onCollision);
		UNDEFVIRT(getSurfaceFriction);
		UNDEFVIRT(getBounceFriction);

		MEMBERFN(void, setHidden, (bool hidden), (hidden), 0x95BD0);

		GETTERFNSIMP(bool, getHiddenGetter, 0x764);

		GETTERFNSIMP(TSThread*, getThread1, 0x2C0);

		GETTERFNSIMP(Thread::State, getThread1State, 0xB1 * 4);
		SETTERFN(Thread::State, setThread1State, 0xB1 * 4);

		GETTERFNSIMP(bool, getThread1Forward, 0x2D1);
		SETTERFN(bool, setThread1Forward, 0x2D1);

		GETTERFNSIMP(bool, getThread1AtEnd, 0x2D0);
		SETTERFN(bool, setThread1AtEnd, 0x2D0);

		SETTERFN(TSThread*, setThread1, 0x2C0);

		GETTERFNSIMP(TSShapeInstance*, getTSShapeInstance, 0x1AB * 4);
	};


	class Item : public ShapeBase
	{

	};

	// TODO: Convert this to use VIRTFN
	/*class DisplayDevice
	{
	public:
		virtual void initDevice() = 0;
		virtual bool activate(U32 width, U32 height, U32 bpp, bool fullScreen) = 0;
		virtual void shutdown() = 0;
		virtual bool setScreenMode(U32 width, U32 height, U32 bpp, bool fullScreen, bool forceIt = false, bool repaint = true) = 0;
		virtual bool setResolution(U32 width, U32 height, U32 bpp) = 0;
		virtual bool toggleFullScreen() = 0;
		virtual void swapBuffers() = 0;
		virtual const char *getDriverInfo() = 0;
		virtual bool getGammaCorrection(F32 &g) = 0;
		virtual bool setGammaCorrection(F32 g) = 0;
		virtual bool setVerticalSync(bool on) = 0;
	};

	class OpenGLDevice : public DisplayDevice
	{
	};*/

	class Marble: public ShapeBase
	{
	public:
		GETTERFNSIMP(bool, getOOB, 0xA45);
		SETTERFN(bool, setOOB, 0xA45);

		GETTERFNSIMP(Point3D, getVelocity, 0x9EC);
		SETTERFN(Point3D, setVelocity, 0x9EC);
		GETTERFNSIMP(Point3D, getAngularVelocity, 0xA1C);
		SETTERFN(Point3D, setAngularVelocity, 0xA1C);

		GETTERFNSIMP(F32, getCameraYaw, 0xA34);
		SETTERFN(F32, setCameraYaw, 0xA34);
		GETTERFNSIMP(F32, getCameraPitch, 0xA38);
		SETTERFN(F32, setCameraPitch, 0xA38);

		GETTERFNSIMP(F32, getCollisionRadius, 0x9D0);
		SETTERFN(F32, setCollisionRadius, 0x9D0);

		GETTERFNSIMP(bool, getControllable, 0xA44);
		SETTERFN(bool, setControllable, 0xA44);

        MEMBERFN(void, setPositionSimple, (const Point3D &position), (position), 0x2541A0);
	};
	
	class Camera: public ShapeBase
	{
	};

	class InteriorInstance : public SceneObject
	{
	};

	class TSStatic : public SceneObject
	{
	};

	struct Collision
	{
		SceneObject* object;
		Point3F point;
		VectorF normal;
		BaseMatInstance* material;

		// Face and Face dot are currently only set by the extrudedPolyList
		// clipper.  Values are otherwise undefined.
		U32 face;                  // Which face was hit
		F32 faceDot;               // -Dot of face with poly normal
		F32 distance;

		Collision() :
			object(NULL),
			material(NULL)
		{
		}
	};

	/// Extension of the collision structure to allow use with raycasting.
	/// @see Collision
	struct RayInfo : public Collision
	{
		// The collision struct has object, point, normal & material.

		/// Distance along ray to contact point.
		F32 t;

		/// Set the point of intersection according to t and the given ray.
		///
		/// Several pieces of code will not use ray information but rather rely
		/// on contact points directly, so it is a good thing to always set
		/// this in castRay functions.
		void setContactPoint(const Point3F& start, const Point3F& end)
		{
			Point3F startToEnd = end - start;
			startToEnd *= t;
			point = startToEnd + start;
		}
	};

	class Container
	{
	public:
		MEMBERFN(bool, castRay, (const Point3F &start, const Point3F &end, U32 mask, RayInfo *info), (start, end, mask, info), TGEADDR_CONTAINER_CASTRAY);
	};

	class Stream
	{
	public:
		VTABLE(TGEOFF_STREAM_VTABLE);
		
		/// Status constants for the stream
		enum StreamStatus
		{
			Ok = 0,      ///< Ok!
			IOError,     ///< Read or Write error
			EOS,         ///< End of Stream reached (mostly for reads)
			IllegalCall, ///< An unsupported operation used. Always w/ accompanied by AssertWarn
			Closed,      ///< Tried to operate on a closed stream (or detached filter)
			UnknownError ///< Catchall
		};
		
		GETTERFNSIMP(StreamStatus, getStatus, TGEOFF_STREAM_STATUS);

		VIRTDTOR(~Stream, TGEVIRT_STREAM_DTOR);
		VIRTFN(bool, _read, (U32 size, void *buf), (size, buf), TGEVIRT_STREAM__READ);
		VIRTFN(bool, _write, (U32 size, const void *buf), (size, buf), TGEVIRT_STREAM__WRITE);
		VIRTFN(bool, hasCapability, (int capability), (capability), TGEVIRT_STREAM_HASCAPABILITY);
		VIRTFNSIMP(U32, getPosition, TGEVIRT_STREAM_GETPOSITION);
		VIRTFN(bool, setPosition, (U32 pos), (pos), TGEVIRT_STREAM_SETPOSITION);
		VIRTFNSIMP(U32, getStreamSize, TGEVIRT_STREAM_GETSTREAMSIZE);
		VIRTFN(void, readString, (char *str), (str), TGEVIRT_STREAM_READSTRING);
		VIRTFN(void, writeString, (const char *str, S32 maxLength), (str, maxLength), TGEVIRT_STREAM_WRITESTRING);
	};

	class FileStream : public Stream
	{
	public:
		MEMBERFN(bool, open, (const char *path, int accessMode), (path, accessMode), TGEADDR_FILESTREAM_OPEN);
	};

	class File
	{
	public:
		/// What is the status of our file handle?
		enum FileStatus
		{
			Ok = 0,      ///< Ok!
			IOError,     ///< Read or Write error
			EOS,         ///< End of Stream reached (mostly for reads)
			IllegalCall, ///< An unsupported operation used. Always accompanied by AssertWarn
			Closed,      ///< Tried to operate on a closed stream (or detached filter)
			UnknownError ///< Catchall
		};

		/// How are we accessing the file?
		enum AccessMode
		{
			Read = 0,       ///< Open for read only, starting at beginning of file.
			Write = 1,      ///< Open for write only, starting at beginning of file; will blast old contents of file.
			ReadWrite = 2,  ///< Open for read-write.
			WriteAppend = 3 ///< Write-only, starting at end of file.
		};

		/// Flags used to indicate what we can do to the file.
		enum Capability
		{
			FileRead = 1 << 0,
			FileWrite = 1 << 1
		};
		
		MEMBERFN(void, setStatus, (FileStatus status), (status), TGEADDR_FILE_SETSTATUS_1); // Technically supposed to be protected
		GETTERFNSIMP(void*, getHandle, TGEOFF_FILE_HANDLE);
		SETTERFN(void*, setHandle, TGEOFF_FILE_HANDLE);
		GETTERFNSIMP(Capability, getCapabilities, TGEOFF_FILE_CAPABILITIES);
		SETTERFN(Capability, setCapabilities, TGEOFF_FILE_CAPABILITIES);

		MEMBERFN(FileStatus, open, (const char *filename, const AccessMode openMode), (filename, openMode), TGEADDR_FILE_OPEN);
		MEMBERFNSIMP(U32, getPosition, TGEADDR_FILE_GETPOSITION);
		MEMBERFN(FileStatus, setPosition, (S32 position, bool absolutePos), (position, absolutePos), TGEADDR_FILE_SETPOSITION);
		MEMBERFNSIMP(U32, getSize, TGEADDR_FILE_GETSIZE);
		MEMBERFNSIMP(FileStatus, flush, TGEADDR_FILE_FLUSH);
		MEMBERFNSIMP(FileStatus, close, TGEADDR_FILE_CLOSE);
		MEMBERFNSIMP(FileStatus, getStatus, TGEADDR_FILE_GETSTATUS);
		MEMBERFN(FileStatus, read, (U32 size, char *dst, U32 *bytesRead), (size, dst, bytesRead), TGEADDR_FILE_READ);
		MEMBERFN(FileStatus, write, (U32 size, const char *src, U32 *bytesWritten), (size, src, bytesWritten), TGEADDR_FILE_WRITE);
	};

	class BitStream : public Stream
	{
	public:
		MEMBERFN(void, writeInt, (S32 value, S32 bitCount), (value, bitCount), TGEADDR_BITSTREAM_WRITEINT);
		MEMBERFN(S32, readInt, (S32 bitCount), (bitCount), TGEADDR_BITSTREAM_READINT);
	};

	class ResManager
	{
	public:
		MEMBERFN(Stream*, openStream, (const char *path), (path), TGEADDR_RESMANAGER_OPENSTREAM_STR);
		MEMBERFN(Stream*, openStream, (ResourceObject *obj), (obj), TGEADDR_RESMANAGER_OPENSTREAM_RESOURCEOBJECT);
		MEMBERFN(void, closeStream, (Stream *stream), (stream), TGEADDR_RESMANAGER_CLOSESTREAM);
		MEMBERFN(ResourceObject*, find, (const char *path), (path), TGEADDR_RESMANAGER_FIND);
		MEMBERFN(U32, getSize, (const char *path), (path), TGEADDR_RESMANAGER_GETSIZE);
		MEMBERFN(bool, getCrc, (const char *path, U32 &crc, U32 initialValue), (path, crc, initialValue), TGEADDR_RESMANAGER_GETCRC);
		MEMBERFN(void, searchPath, (const char *path), (path), TGEADDR_RESMANAGER_SEARCHPATH);
		MEMBERFN(bool, setModZip, (const char *path), (path), TGEADDR_RESMANAGER_SETMODZIP);
	};

	class ConnectionProtocol
	{
	public:
		// TODO: Clean this up, this is just so casting SimObject* to NetConnection* works properly
		unsigned char z_fields[0x9C];
		virtual void z_placeholder() = 0; // Force virtual inheritance
	};
	
	GLOBALVAR(TGE::SimObject*, mServerConnection, TGEADDR_MSERVERCONNECTION);

	class NetConnection : public ConnectionProtocol, public SimObject
	{
	public:
		static NetConnection *getConnectionToServer()
		{
			return static_cast<TGE::NetConnection*>(mServerConnection);
		}
	};

	class GameConnection : public NetConnection
	{
	};

	class PathedInterior : public GameBase
	{
	public:
		MEMBERFN(void, advance, (double delta), (delta), TGEADDR_PATHEDINTERIOR_ADVANCE);
		MEMBERFN(void, computeNextPathStep, (U32 delta), (delta), TGEADDR_PATHEDINTERIOR_COMPUTENEXTPATHSTEP);

		MEMBERFNSIMP(Point3F, getVelocity, 0x24B690);

		GETTERFNSIMP(MatrixF, getBaseTransform, 0x328);
		SETTERFN(MatrixF, setBaseTransform, 0x328);

        GETTERFNSIMP(F64, getPathPosition, 0x378);
        SETTERFN(F64, setPathPosition, 0x378);

        GETTERFNSIMP(S32, getTargetPosition, 0x380);
        SETTERFN(S32, setTargetPosition, 0x380);

		GETTERFNSIMP(Point3F, getOffset, 0xA1 * 4);
		SETTERFN(Point3F, setOffset, 0xA1 * 4);

		GETTERFNSIMP(U32, getPathKey, 0xDD * 4);
		SETTERFN(U32, setPathKey, 0xDD * 4);
        MEMBERFNSIMP(U32, getPathKey2, 0x24B6C0);
        MEMBERFN(void, processTick, (const Move *move), (move), 0x24B560);
    };

	class AbstractClassRep
	{
	public:
		GETTERFNSIMP(const char*, getClassName, TGEOFF_ABSTRACTCLASSREP_CLASSNAME);
	};

	class _StringTable
	{
	public:
		MEMBERFN(const char*, insert, (const char *string, bool caseSens), (string, caseSens), TGEADDR__STRINGTABLE_INSERT);
	};

	// Event types
	enum EventType
	{
		TimeEventType = 3,
	};

	// Event base structure
	struct Event
	{
		U16 type;
		U16 size;

		Event()
		{
			size = sizeof(Event);
		}
	};

	// Structure for time events
	struct TimeEvent : public Event
	{
		U32 deltaTime;

		TimeEvent()
		{
			type = TimeEventType;
			size = sizeof(TimeEvent);
		}
	};

	class GameInterface
	{
	public:
		VTABLE(TGEOFF_GAMEINTERFACE_VTABLE);

		VIRTFN(void, postEvent, (Event &ev), (ev), TGEVIRT_GAMEINTERFACE_POSTEVENT);
	};

    class PathManager {
    public:
        MEMBERFN(U32, getPathTotalTime, (U32 id), (id), 0x194D70);
        MEMBERFN(void, getPathPosition, (U32 id, F64 msPosition, Point3F &rPosition), (id, msPosition, rPosition), 0x194E70);
    };

	// Console enums
	namespace ConsoleLogEntry
	{
		enum Level
		{
			Normal = 0,
			Warning,
			Error,
			NUM_CLASS
		};

		enum Type
		{
			General = 0,
			Assert,
			Script,
			GUI,
			Network,
			NUM_TYPE
		};
	}

	namespace Con
	{
		// Initialization
		FN(void, init, (), TGEADDR_CON_INIT);
		
		// Logging
		FN(void, printf, (const char *fmt, ...), TGEADDR_CON_PRINTF);

		// _printf is fastcall on Mac
#ifndef __APPLE__
		FN(void, _printf, (ConsoleLogEntry::Level level, ConsoleLogEntry::Type type, const char *fmt, va_list argptr), TGEADDR_CON__PRINTF);
#else
		FASTCALLFN(void, _printf, (ConsoleLogEntry::Level level, ConsoleLogEntry::Type type, const char *fmt, va_list argptr), TGEADDR_CON__PRINTF);
#endif

		OVERLOAD_PTR {
			OVERLOAD_FN(void, (const char *fmt, ...),                             TGEADDR_CON_WARNF_1V);
			OVERLOAD_FN(void, (ConsoleLogEntry::Type type, const char *fmt, ...), TGEADDR_CON_WARNF_2V);
		} warnf;
		OVERLOAD_PTR {
			OVERLOAD_FN(void, (const char *fmt, ...),                             TGEADDR_CON_ERRORF_1V);
			OVERLOAD_FN(void, (ConsoleLogEntry::Type type, const char *fmt, ...), TGEADDR_CON_ERRORF_2V);
		} errorf;

		// addCommand()
		OVERLOAD_PTR {
			OVERLOAD_FN(void, (const char *name, StringCallback cb, const char *usage, S32 minArgs, S32 maxArgs),                     TGEADDR_CON_ADDCOMMAND_5_STRING);
			OVERLOAD_FN(void, (const char *name, VoidCallback cb, const char *usage, S32 minArgs, S32 maxArgs),                       TGEADDR_CON_ADDCOMMAND_5_VOID);
			OVERLOAD_FN(void, (const char *name, IntCallback cb, const char *usage, S32 minArgs, S32 maxArgs),                        TGEADDR_CON_ADDCOMMAND_5_INT);
			OVERLOAD_FN(void, (const char *name, FloatCallback cb, const char *usage, S32 minArgs, S32 maxArgs),                      TGEADDR_CON_ADDCOMMAND_5_FLOAT);
			OVERLOAD_FN(void, (const char *name, BoolCallback cb, const char *usage, S32 minArgs, S32 maxArgs),                       TGEADDR_CON_ADDCOMMAND_5_BOOL);
			OVERLOAD_FN(void, (const char *nsName, const char *name, StringCallback cb, const char *usage, S32 minArgs, S32 maxArgs), TGEADDR_CON_ADDCOMMAND_6_STRING);
			OVERLOAD_FN(void, (const char *nsName, const char *name, VoidCallback cb, const char *usage, S32 minArgs, S32 maxArgs),   TGEADDR_CON_ADDCOMMAND_6_VOID);
			OVERLOAD_FN(void, (const char *nsName, const char *name, IntCallback cb, const char *usage, S32 minArgs, S32 maxArgs),    TGEADDR_CON_ADDCOMMAND_6_INT);
			OVERLOAD_FN(void, (const char *nsName, const char *name, FloatCallback cb, const char *usage, S32 minArgs, S32 maxArgs),  TGEADDR_CON_ADDCOMMAND_6_FLOAT);
			OVERLOAD_FN(void, (const char *nsName, const char *name, BoolCallback cb, const char *usage, S32 minArgs, S32 maxArgs),   TGEADDR_CON_ADDCOMMAND_6_BOOL);
		} addCommand;

		OVERLOAD_PTR{
			OVERLOAD_FN(const char*, (SimObject * obj, int argc, ...), 0x3B420);
			OVERLOAD_FN(const char*, (int argc, ...), TGEADDR_CON_EXECUTEF);
		} executef;

		// Variables
		FN(const char *, getVariable, (const char *name), 0x03BA20);
		FN(bool, getBoolVariable, (const char *name), 0x03BD50);
		FN(S32, getIntVariable, (const char* name), 0x3BD10);
		FN(F32, getFloatVariable, (const char* name), 0x3BCD0);

		FN(void, setVariable,      (const char *name, const char *value), TGEADDR_CON_SETVARIABLE);
		FN(void, setLocalVariable, (const char *name, const char *value), TGEADDR_CON_SETLOCALVARIABLE);
		FN(void, setBoolVariable,  (const char *name, bool value),        TGEADDR_CON_SETBOOLVARIABLE);
		FN(void, setIntVariable,   (const char *name, S32 value),         TGEADDR_CON_SETINTVARIABLE);
		FN(void, setFloatVariable, (const char *name, F32 value),         TGEADDR_CON_SETFLOATVARIABLE);

		// Misc
		FN(const char*, execute,              (S32 argc, const char *argv[]),                        TGEADDR_CON_EXECUTE);
		FN(const char*, evaluate,             (const char *string, bool echo, const char *fileName), TGEADDR_CON_EVALUATE);
		FN(const char*, evaluatef,            (const char* string, ...),                             TGEADDR_CON_EVALUATEF);
		FN(char*,       getReturnBuffer,      (U32 bufferSize),                                      TGEADDR_CON_GETRETURNBUFFER);
		FN(bool,        expandScriptFilename, (char *filename, U32 size, const char *src),           TGEADDR_CON_EXPANDSCRIPTFILENAME);
	}

	namespace Platform
	{
		FN(bool, dumpPath, (const char *path, Vector<FileInfo>& fileVector), TGEADDR_PLATFORM_DUMPPATH);
		FN(const char*, getWorkingDirectory, (), TGEADDR_PLATFORM_GETWORKINGDIRECTORY);
		FN(bool, isSubDirectory, (const char *parent, const char *child), TGEADDR_PLATFORM_ISSUBDIRECTORY);
		FN(bool, getFileTimes, (const char *path, FileTime *createTime, FileTime *modifyTime), TGEADDR_PLATFORM_GETFILETIMES);
	}

	namespace Namespace
	{
		FN(void, init, (), TGEADDR_NAMESPACE_INIT);

		FN(Namespace*, find, (const char* name, const char* package), 0x33710);

		GLOBALVAR(void*, gEvalState, 0x2FF2C0);

		class NamespaceEntry
		{
		public:
			Namespace* mNamespace;
			NamespaceEntry* mNext;
			const char* mFunctionName;
			S32 mType;
			S32 mMinArgs;
			S32 mMaxArgs;
			const char* mUsage;
			const char* mPackage;

			MEMBERFN(const char*, execute, (S32 argc, const char** argv, void* state), (argc, argv, state), 0x32BF0);
		};

		class Namespace
		{
		public:
			const char* mName;
			const char* mPackage;

			Namespace* mParent;
			Namespace* mNext;
			AbstractClassRep* mClassRep;
			U32 mRefCountToParent;
			NamespaceEntry* mEntryList;


		public:
			MEMBERFN(NamespaceEntry*, lookup, (const char* name), (name), 0x335A0);
		};
	}

	namespace ParticleEngine
	{
		// Initialization
		FN(void, init, (), TGEADDR_PARTICLEENGINE_INIT);
		FN(void, destroy, (), TGEADDR_PARTICLEENGINE_DESTROY);
	}

	namespace Sim
	{
		FN(SimObject*, findObject, (const char *name), TGEADDR_SIM_FINDOBJECT);
		FN(SimObject*, findObject_int, (SimObjectId id), 0x025480);
		GLOBALVAR(U32, gCurrentTime, 0x2FD870);
		FN(void, cancelEvent, (U32 eventSequence), 0x025360);
	}

	namespace Net
	{
		enum Error
		{
			NoError,
			WrongProtocolType,
			InvalidPacketProtocol,
			WouldBlock,
			NotASocket,
			UnknownError
		};

		FN(bool, init, (), TGEADDR_NET_INIT);
		FN(Error, bind, (NetSocket socket, U16 port), TGEADDR_NET_BIND);
	}

	namespace TimeManager
	{
		FN(void, process, (), TGEADDR_TIMEMANAGER_PROCESS);
	}

	namespace Members
	{
		/*namespace OpenGLDevice
		{
			RAWMEMBERFNSIMP(TGE::OpenGLDevice, void, initDevice, 0x4033BE);
		}*/

		namespace ShapeBase
		{
			RAWMEMBERFN(TGE::ShapeBase, U32, packUpdate, (NetConnection *connection, U32 mask, TGE::BitStream *stream), 0x09E7A0);
			RAWMEMBERFN(TGE::ShapeBase, void, unpackUpdate, (NetConnection *connection, TGE::BitStream *stream), 0xA46C0);
			RAWMEMBERFN(TGE::ShapeBase, void, updateThread, (Thread& st), 0x9B2C0);
			RAWMEMBERFNSIMP(TGE::ShapeBase, bool, onAdd, 0x0A2AF0);
			RAWMEMBERFNSIMP(TGE::ShapeBase, void, onRemove, 0x9FB70);
		}

		namespace Marble
		{
			RAWMEMBERFN(TGE::Marble, void, doPowerUp, (int id), TGEADDR_MARBLE_DOPOWERUP);
			RAWMEMBERFN(TGE::Marble, void, advancePhysics, (Move *move, U32 delta), TGEADDR_MARBLE_ADVANCEPHYSICS);
		}

		namespace GameBase
		{
			RAWMEMBERFN(TGE::GameBase, U32, packUpdate, (TGE::NetConnection *conn, U32 mask, TGE::BitStream *stream), TGEADDR_GAMEBASE_PACKUPDATE);
			RAWMEMBERFN(TGE::GameBase, void, unpackUpdate, (TGE::NetConnection *conn, TGE::BitStream *stream), TGEADDR_GAMEBASE_UNPACKUPDATE);
		}

		namespace TSStatic
		{
			RAWMEMBERFN(TGE::TSStatic, U32, packUpdate, (TGE::NetConnection *conn, U32 mask, TGE::BitStream *stream), TGEADDR_TSSTATIC_PACKUPDATE);
			RAWMEMBERFN(TGE::TSStatic, void, unpackUpdate, (TGE::NetConnection *conn, TGE::BitStream *stream), TGEADDR_TSSTATIC_UNPACKUPDATE);
		}

		namespace InteriorInstance
		{
			RAWMEMBERFN(TGE::InteriorInstance, U32, packUpdate, (TGE::NetConnection *conn, U32 mask, TGE::BitStream *stream), TGEADDR_INTERIORINSTANCE_PACKUPDATE);
			RAWMEMBERFN(TGE::InteriorInstance, void, unpackUpdate, (TGE::NetConnection *conn, TGE::BitStream *stream), TGEADDR_INTERIORINSTANCE_UNPACKUPDATE);
		}

		namespace FileStream
		{
			RAWMEMBERFN(TGE::FileStream, bool, open, (const char *path, int accessMode), TGEADDR_FILESTREAM_OPEN);
		}

		namespace PathedInterior
		{
			RAWMEMBERFN(TGE::PathedInterior, void, advance, (double delta), TGEADDR_PATHEDINTERIOR_ADVANCE);
			RAWMEMBERFN(TGE::PathedInterior, void, computeNextPathStep, (U32 delta), TGEADDR_PATHEDINTERIOR_COMPUTENEXTPATHSTEP);
			RAWMEMBERFN(TGE::PathedInterior, void, processTick, (const TGE::Move *move), 0x24B560);
			RAWMEMBERFN(TGE::PathedInterior, U32, packUpdate, (TGE::NetConnection* con, U32 mask, TGE::BitStream* stream), 0x024D4A0);
			RAWMEMBERFN(TGE::PathedInterior, void, unpackUpdate, (TGE::NetConnection* con, TGE::BitStream* stream), 0x24D740);
			RAWMEMBERFNSIMP(TGE::PathedInterior, bool, onAdd, 0x24C3B0);
			RAWMEMBERFNSIMP(TGE::PathedInterior, void, onRemove, 0x24BA30);
		}

		namespace AbstractClassRep
		{
			FN(void, initialize, (), TGEADDR_ABSTRACTCLASSREP_INITIALIZE);
		}

		namespace Item
		{
			RAWMEMBERFN(TGE::Item, void, advanceTime, (F32 dt), 0x0D8590);
		}

		namespace TSShapeInstance
		{
			RAWMEMBERFN(TGE::TSShapeInstance, void, advanceTime, (F32 delta), 0x1B2910);
		}

		namespace File
		{
			RAWMEMBERFN(TGE::File, TGE::File::FileStatus, open, (const char *filename, const TGE::File::AccessMode openMode), TGEADDR_FILE_OPEN);
			RAWMEMBERFNSIMP(TGE::File, U32, getPosition, TGEADDR_FILE_GETPOSITION);
			RAWMEMBERFN(TGE::File, TGE::File::FileStatus, setPosition, (S32 position, bool absolutePos), TGEADDR_FILE_SETPOSITION);
			RAWMEMBERFNSIMP(TGE::File, U32, getSize, TGEADDR_FILE_GETSIZE);
			RAWMEMBERFNSIMP(TGE::File, TGE::File::FileStatus, flush, TGEADDR_FILE_FLUSH);
			RAWMEMBERFNSIMP(TGE::File, TGE::File::FileStatus, close, TGEADDR_FILE_CLOSE);
			RAWMEMBERFNSIMP(TGE::File, TGE::File::FileStatus, getStatus, TGEADDR_FILE_GETSTATUS);
			RAWMEMBERFN(TGE::File, void, setStatus, (TGE::File::FileStatus status), TGEADDR_FILE_SETSTATUS_1); // Technically supposed to be protected
			RAWMEMBERFN(TGE::File, TGE::File::FileStatus, read, (U32 size, char *dst, U32 *bytesRead), TGEADDR_FILE_READ);
			RAWMEMBERFN(TGE::File, TGE::File::FileStatus, write, (U32 size, const char *src, U32 *bytesWritten), TGEADDR_FILE_WRITE);
			RAWMEMBERFNSIMP(TGE::File, void, destructor_, TGEADDR_FILE_DTOR);
		}
		
		namespace BitStream
		{
			RAWMEMBERFN(TGE::BitStream, void, writeInt, (S32 value, S32 bitCount), TGEADDR_BITSTREAM_WRITEINT);
			RAWMEMBERFN(TGE::BitStream, S32, readInt, (S32 bitCount), TGEADDR_BITSTREAM_READINT);
		}
		
		namespace Camera
		{
			RAWMEMBERFN(TGE::Camera, void, advancePhysics, (const TGE::Move *move, U32 delta), TGEADDR_CAMERA_ADVANCEPHYSICS);
		}
		
		namespace ResManager
		{
			// TODO: Allow overloading raw member pointers
			/*RAWMEMBERFN(TGE::ResManager, Stream*, openStream, (const char *path), 0x407E37);
			RAWMEMBERFN(TGE::ResManager, Stream*, openStream, (ResourceObject *obj), 0x4079EB);*/
			RAWMEMBERFN(TGE::ResManager, void, closeStream, (Stream *stream), TGEADDR_RESMANAGER_CLOSESTREAM);
			RAWMEMBERFN(TGE::ResManager, ResourceObject*, find, (const char *path), TGEADDR_RESMANAGER_FIND);
			RAWMEMBERFN(TGE::ResManager, U32, getSize, (const char *path), TGEADDR_RESMANAGER_GETSIZE);
			RAWMEMBERFN(TGE::ResManager, bool, getCrc, (const char *path, U32 &crc, U32 initialValue), TGEADDR_RESMANAGER_GETCRC);
			RAWMEMBERFN(TGE::ResManager, void, searchPath, (const char *path), TGEADDR_RESMANAGER_SEARCHPATH);
			RAWMEMBERFN(TGE::ResManager, bool, setModZip, (const char *path), TGEADDR_RESMANAGER_SETMODZIP);
		}

		namespace Marble
        {
            RAWMEMBERFN(TGE::Marble, void, setPositionSimple, (const Point3D &position), 0x2541A0);
        }

        namespace PathManager
        {
            RAWMEMBERFN(TGE::PathManager, U32, getPathTotalTime, (U32 id), 0x194D70);
        }
	}

	FN(void, shutdownGame, (), TGEADDR_SHUTDOWNGAME);
	FN(void, clientProcess, (U32 timeDelta), TGEADDR_CLIENTPROCESS);

	// Platform functions
	FN(int, dSprintf, (char *buffer, size_t bufferSize, const char *format, ...), TGEADDR_DSPRINTF);
	FN(int, dVsprintf, (char *buffer, size_t maxSize, const char *format, void *args), TGEADDR_DVSPRINTF);
	FN(void, dFree, (void *ptr), TGEADDR_DFREE);
	FN(void, dQsort, (void *base, U32 nelem, U32 width, int (QSORT_CALLBACK *fcmp)(const void*, const void*)), TGEADDR_DQSORT);
	FN(bool, VectorResize, (U32 *aSize, U32 *aCount, void **arrayPtr, U32 newCount, U32 elemSize), TGEADDR_VECTORRESIZE);

	// Global variables
	GLOBALVAR(Container, gClientContainer, TGEADDR_GCLIENTCONTAINER);
	GLOBALVAR(Container, gServerContainer, TGEADDR_GSERVERCONTAINER);
	GLOBALVAR(_StringTable*, StringTable, TGEADDR_STRINGTABLE);
	GLOBALVAR(ResManager*, ResourceManager, TGEADDR_RESOURCEMANAGER);
	GLOBALVAR(GameInterface*, Game, TGEADDR_GAME);
	GLOBALVAR(GuiCanvas*, Canvas, TGEADDR_CANVAS);
    GLOBALVAR(PathManager *, gClientPathManager, 0x2DB948);
    GLOBALVAR(PathManager *, gServerPathManager, 0x2DB944);
}

// ConsoleFunction() can't be used from inside PluginLoader.dll without crashes
#ifndef IN_PLUGIN_LOADER

namespace TGE
{
	// Psuedo class used to implement the ConsoleFunction macro.
	class _ConsoleConstructor
	{
	public:
		_ConsoleConstructor(const char *name, StringCallback cb, const char *usage, S32 minArgs, S32 maxArgs)
		{
			Con::addCommand(name, cb, usage, minArgs, maxArgs);
		}

		_ConsoleConstructor(const char *name, VoidCallback cb, const char *usage, S32 minArgs, S32 maxArgs)
		{
			Con::addCommand(name, cb, usage, minArgs, maxArgs);
		}

		_ConsoleConstructor(const char *name, IntCallback cb, const char *usage, S32 minArgs, S32 maxArgs)
		{
			Con::addCommand(name, cb, usage, minArgs, maxArgs);
		}

		_ConsoleConstructor(const char *name, FloatCallback cb, const char *usage, S32 minArgs, S32 maxArgs)
		{
			Con::addCommand(name, cb, usage, minArgs, maxArgs);
		}

		_ConsoleConstructor(const char *name, BoolCallback cb, const char *usage, S32 minArgs, S32 maxArgs)
		{
			Con::addCommand(name, cb, usage, minArgs, maxArgs);
		}

		_ConsoleConstructor(const char *nsName, const char *name, StringCallback cb, const char *usage, S32 minArgs, S32 maxArgs)
		{
			Con::addCommand(nsName, name, cb, usage, minArgs, maxArgs);
		}

		_ConsoleConstructor(const char *nsName, const char *name, VoidCallback cb, const char *usage, S32 minArgs, S32 maxArgs)
		{
			Con::addCommand(nsName, name, cb, usage, minArgs, maxArgs);
		}

		_ConsoleConstructor(const char *nsName, const char *name, IntCallback cb, const char *usage, S32 minArgs, S32 maxArgs)
		{
			Con::addCommand(nsName, name, cb, usage, minArgs, maxArgs);
		}

		_ConsoleConstructor(const char *nsName, const char *name, FloatCallback cb, const char *usage, S32 minArgs, S32 maxArgs)
		{
			Con::addCommand(nsName, name, cb, usage, minArgs, maxArgs);
		}

		_ConsoleConstructor(const char *nsName, const char *name, BoolCallback cb, const char *usage, S32 minArgs, S32 maxArgs)
		{
			Con::addCommand(nsName, name, cb, usage, minArgs, maxArgs);
		}
	};
}

// Defines a console function.
#define ConsoleFunction(name, returnType, minArgs, maxArgs, usage)                         \
	static returnType c##name(TGE::SimObject *, S32, const char **argv);                   \
	static TGE::_ConsoleConstructor g##name##obj(#name, c##name, usage, minArgs, maxArgs); \
	static returnType c##name(TGE::SimObject *, S32 argc, const char **argv)

// Hacks to handle properly returning different value types from console methods
#define CONMETHOD_NULLIFY(val)
#define CONMETHOD_RETURN_const return (const
#define CONMETHOD_RETURN_S32   return (S32
#define CONMETHOD_RETURN_F32   return (F32
#define CONMETHOD_RETURN_void  CONMETHOD_NULLIFY(void
#define CONMETHOD_RETURN_bool  return (bool

// Defines a console method.
#define ConsoleMethod(className, name, type, minArgs, maxArgs, usage)                                                                \
	static type c##className##name(TGE::className *, S32, const char **argv);                                                        \
	static type c##className##name##caster(TGE::SimObject *object, S32 argc, const char **argv) {                                    \
		if (!object)                                                                                                                 \
			TGE::Con::warnf("Object passed to " #name " is null!");                                                                  \
		CONMETHOD_RETURN_##type ) c##className##name(static_cast<TGE::className*>(object), argc, argv);                              \
	};                                                                                                                               \
	static TGE::_ConsoleConstructor g##className##name##obj(#className, #name, c##className##name##caster, usage, minArgs, maxArgs); \
	static type c##className##name(TGE::className *object, S32 argc, const char **argv)

#endif // IN_PLUGIN_LOADER

#endif // TORQUELIB_TGE_H