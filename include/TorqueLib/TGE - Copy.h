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

// Determine macros and addresses to use based on host OS
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
	struct Move;
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
	class ConsoleObject
	{
	public:
		VTABLE(TGEOFF_CONSOLEOBJECT_VTABLE);
		
		UNDEFVIRT(getClassRep);
		VIRTDTOR(~ConsoleObject, TGEVIRT_CONSOLEOBJECT_DESTRUCTOR);
	};

	class SimObject: public ConsoleObject
	{
	public:
		GETTERFNSIMP(SimObjectId, getId, TGEOFF_SIMOBJECT_ID);
		MEMBERFNSIMP(const char*, getIdString, TGEADDR_SIMOBJECT_GETIDSTRING);
		MEMBERFN(void, setHidden, (bool hidden), (hidden), TGEADDR_SIMOBJECT_SETHIDDEN);

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
		UNDEFVIRT(getEditorClassName);
		UNDEFVIRT(findObject);
		UNDEFVIRT(write);
		UNDEFVIRT(registerLights);
	};

	class SimDataBlock : public SimObject
	{

	};

	class AudioProfile : public SimDataBlock
	{

	};

	class SimObjectList : public VectorPtr<SimObject *>
	{
		static S32 QSORT_CALLBACK compareId(const void *a, const void *b);
	public:
		void pushBack(SimObject *);
		void pushBackForce(SimObject *);
		void pushFront(SimObject *);
		void remove(SimObject *);

		SimObject *at(S32 index) const {
			if (index >= 0 && index < size())
				return (*this)[index];
			return NULL;
		}
		void removeStable(SimObject *pObject);
		void sortId();
	};
	class SimSet : public SimObject
	{
		char unknown[0x2c];
	public:
		SimObjectList objectList;
		void *mMutex;

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
		SimObject **hashTable;
		S32 hashTableSize;
		S32 hashEntryCount;

		void *mutex;
	public:
		void insert(SimObject* obj);
		void remove(SimObject* obj);
		SimObject* find(const char * name);

		SimNameDictionary();
		~SimNameDictionary();
	};
	class SimGroup : public SimSet
	{
		SimNameDictionary nameDictionary;
	public:
	};

	class GuiControl : public SimGroup
	{
	public:
		GETTERFNSIMP(RectI, getBounds, 0x58);
		GETTERFNSIMP(Point2I, getPosition, 0x58);
		GETTERFNSIMP(Point2I, getExtent, 0x60);
		SETTERFN(Point2I, setExtent, 0x60);
	};

	class GuiCanvas : public GuiControl
	{

	};


	class NetObject: public SimObject
	{
	public:
		UNDEFVIRT(getUpdatePriority);
		UNDEFVIRT(onCameraScopeQuery);
		inline bool isServerObject() { return (*((U32 *)((U8 *)this + 0x40)) & 2) == 0; }
		inline bool isClientObject() { return (*((U32 *)((U8 *)this + 0x40)) & 2) != 0; }
	};

	class SceneState;
	class SceneRenderImage;

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
		VIRTFN(void, setTransform, (const MatrixF &transform), (transform), TGEVIRT_SCENEOBJECT_SETTRANSFORM);
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
		VIRTFN(void, renderObject, (SceneState *state, SceneRenderImage *renderImage), (state, renderImage), TGEVIRT_SCENEOBJECT_RENDEROBJECT);
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

		GETTERFN(const MatrixF &, MatrixF, getTransform, TGEOFF_SCENEOBJECT_TRANSFORM);
		GETTERFN(const Box3F &, Box3F, getWorldBox, TGEOFF_SCENEOBJECT_WORLDBOX);
	};

	class GameBase : public SceneObject
	{
	public:
		GETTERFNSIMP(GameConnection*, getControllingClient, TGEOFF_GAMEBASE_CONTROLLINGCLIENT);

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

		MEMBERFNSIMP(void, scriptOnAdd, TGEADDR_GAMEBASE_SCRIPTONADD);
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
		UNDEFVIRT(setHidden);
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
		MEMBERFNSIMP(bool, onAdd, TGEADDR_MARBLE_ONADD);
		MEMBERFN(void, setTransform, (const MatrixF &mat), (mat), TGEADDR_MARBLE_SETTRANSFORM);
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

	class SceneGraph {
	public:
		MEMBERFN(void, renderScene, (unsigned int a), (a), TGEADDR_SCENEGRAPH_RENDERSCENE);
	};

	class PathedInterior : public GameBase
	{
	public:
		MEMBERFN(void, advance, (double delta), (delta), TGEADDR_PATHEDINTERIOR_ADVANCE);
		MEMBERFN(void, computeNextPathStep, (U32 delta), (delta), TGEADDR_PATHEDINTERIOR_COMPUTENEXTPATHSTEP);
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
		U32 elapsedTime;

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

		// Variables
		FN(const char*, getVariable, (const char *name), TGEADDR_CON_GETVARIABLE);
		FN(bool, getBoolVariable, (const char *name), TGEADDR_CON_GETBOOLVARIABLE);
		FN(S32, getIntVariable, (const char *name), TGEADDR_CON_GETINTVARIABLE);
		FN(F32, getFloatVariable, (const char *name), TGEADDR_CON_GETFLOATVARIABLE);
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
	}

	namespace ParticleEngine
	{
		// Initialization
		FN(void, init, (), TGEADDR_PARTICLEENGINE_INIT);
	}

	namespace Sim
	{
		FN(SimObject*, findObject, (const char *name), TGEADDR_SIM_FINDOBJECT);
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

		namespace SimObject
		{
			RAWMEMBERFN(TGE::SimObject, const char *, getDataField, (const char *slotName, const char *array), TGEADDR_SIMOBJECT_GETDATAFIELD);
			RAWMEMBERFN(TGE::SimObject, void, setDataField, (const char *slotName, const char *array, const char *value), TGEADDR_SIMOBJECT_SETDATAFIELD);
		}

		namespace NetObject
		{
			RAWMEMBERFNSIMP(TGE::NetObject, bool, onAdd, TGEADDR_NETOBJECT_ONADD);
			RAWMEMBERFN(TGE::NetObject, U32, packUpdate, (NetConnection *connection, U32 mask, TGE::BitStream *stream), TGEADDR_NETOBJECT_PACKUPDATE);
			RAWMEMBERFN(TGE::NetObject, void, unpackUpdate, (NetConnection *connection, TGE::BitStream *stream), TGEADDR_NETOBJECT_UNPACKUPDATE);
		}

		namespace SceneGraph {
			RAWMEMBERFN(TGE::SceneGraph, void, renderScene, (unsigned int a), TGEADDR_SCENEGRAPH_RENDERSCENE);
		}

		namespace Marble
		{
			RAWMEMBERFN(TGE::Marble, void, doPowerUp, (int id), TGEADDR_MARBLE_DOPOWERUP);
			RAWMEMBERFN(TGE::Marble, void, advancePhysics, (const Move *move, U32 delta), TGEADDR_MARBLE_ADVANCEPHYSICS);
			RAWMEMBERFNSIMP(TGE::Marble, bool, onAdd, TGEADDR_MARBLE_ONADD);
			RAWMEMBERFN(TGE::Marble, void, advanceCamera, (const Move *move, U32 delta), TGEADDR_MARBLE_ADVANCECAMERA);
			RAWMEMBERFN(TGE::Marble, void, setTransform, (const MatrixF &mat), TGEADDR_MARBLE_SETTRANSFORM);
		}

		namespace SceneObject
		{
			RAWMEMBERFN(TGE::SceneObject, void, setTransform, (const MatrixF &mat), TGEADDR_SCENEOBJECT_SETTRANSFORM);
		}
		
		namespace ShapeBase
		{
			RAWMEMBERFN(TGE::ShapeBase, void, renderObject, (void *sceneState, void *sceneRenderImage), TGEADDR_SHAPEBASE_RENDEROBJECT);
		}

		namespace GameBase
		{
			RAWMEMBERFN(TGE::GameBase, U32, packUpdate, (TGE::NetConnection *conn, U32 mask, TGE::BitStream *stream), TGEADDR_GAMEBASE_PACKUPDATE);
			RAWMEMBERFN(TGE::GameBase, void, unpackUpdate, (TGE::NetConnection *conn, TGE::BitStream *stream), TGEADDR_GAMEBASE_UNPACKUPDATE);
			RAWMEMBERFNSIMP(TGE::GameBase, void, scriptOnAdd, TGEADDR_GAMEBASE_SCRIPTONADD);
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
		}

		namespace AbstractClassRep
		{
			FN(void, initialize, (), TGEADDR_ABSTRACTCLASSREP_INITIALIZE);
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
	}

	FN(void, dglSetCanonicalState, (), TGEADDR_DGLSETCANONICALSTATE);
	FN(void, GameRenderWorld, (), TGEADDR_GAMERENDERWORLD);
	FN(void, shutdownGame, (), TGEADDR_SHUTDOWNGAME);
	FN(void, clientProcess, (U32 timeDelta), TGEADDR_CLIENTPROCESS);
	FN(int, alxPlay, (int source), TGEADDR_ALXPLAY);
	FN(int, alxCreateSource, (AudioProfile* sfx, const MatrixF* transform), TGEADDR_ALXCREATESOURCE);
	FN(const char*, ceval, (SimObject *object, int argc, const char **argv), TGEADDR_CEVAL);
	FN(bool, GameGetCameraTransform, (MatrixF *mat, Point3F *pos), TGEADDR_GAMEGETCAMERATRANSFORM);

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
	GLOBALVAR(GuiCanvas *, Canvas, TGEADDR_CANVAS);
	GLOBALVAR(SceneGraph *, gClientSceneGraph, TGEADDR_GCLIENTSCENEGRAPH);
}

// ConsoleFunction() can't be used from inside PluginLoader.dll without crashes
#ifndef IN_PLUGIN_LOADER

namespace TGE
{
	/*OVERLOAD_PTR {
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
		} addCommand;*/

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

// O hackery of hackeries
#define conmethod_return_const              return (const
#define conmethod_return_S32                return (S32
#define conmethod_return_F32                return (F32
#define conmethod_nullify(val)
#define conmethod_return_void               conmethod_nullify(void
#define conmethod_return_bool               return (bool

#define ConsoleMethod(className, name, type, minArgs, maxArgs, usage) \
	static type c##className##name(TGE::className *, S32, const char **argv); \
	static type c##className##name##caster(TGE::SimObject *object, S32 argc, const char **argv) { \
		if (!object) \
			TGE::Con::warnf("Object passed to " #name " is not a " #className "!"); \
		conmethod_return_##type ) c##className##name(static_cast<TGE::className*>(object),argc,argv); \
	}; \
	static TGE::_ConsoleConstructor g##className##name##obj(#className, #name, c##className##name##caster, usage, minArgs, maxArgs); \
	static type c##className##name(TGE::className *object, S32 argc, const char **argv)



#endif // IN_PLUGIN_LOADER

#endif // TORQUELIB_TGE_H
