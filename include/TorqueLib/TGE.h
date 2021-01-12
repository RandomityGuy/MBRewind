// TGE.h: Interface to core TGE functions.
// Usable both from plugins and PluginLoader.dll.

#ifdef __APPLE__
#include "TGE-osx.h"
#else
#ifndef TORQUELIB_TGE_H
#define TORQUELIB_TGE_H

#include <cstdarg>

#include "platform/platform.h"

#include "math/mMath.h"
#include "util/tVector.h"

#ifdef _DEBUG
#define DEBUG_PRINTF(...) TGE::Con::printf(__VA_ARGS__)
#define DEBUG_WARNF(...)  TGE::Con::warnf (__VA_ARGS__)
#define DEBUG_ERRORF(...) TGE::Con::errorf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...) {if (atoi(TGE::Con::getVariable("$DEBUG"))) TGE::Con::printf(__VA_ARGS__);}
#define DEBUG_WARNF(...)
#define DEBUG_ERRORF(...)
#endif

// Determine macros and addresses to use based on host OS
#if defined(_WIN32)
#include "win32/InterfaceMacros-win32.h"
#include "win32/Addresses-win32.h"
#elif defined(__APPLE__)
#include "linux/InterfaceMacros-linux.h"
#include "osx/Addresses-osx.h"
#elif defined(__linux)
#include "linux/InterfaceMacros-linux.h"
#include "linux/Addresses-linux.h"
#endif

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
	class SimDataBlock;
	class NetObject;
	class RayInfo;
	class SceneState;
	class SceneGraph;

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
	class SimObject;

	class BitSet32 {
	public:
		U32 data;
	};

	struct EnumTable {
		S32 size;
		struct Enums {
			S32 index;
			const char* label;
		};
		Enums* table;
	};

	class AbstractClassRep
	{

		VTABLE(0);
	public:
		VIRTDTOR(~AbstractClassRep, TGEVIRT_ABSTRACTCLASSREP_DESTRUCTOR);
		VIRTFNSIMP(SimObject*, create, TGEVIRT_ABSTRACTCLASSREP_CREATE);
		VIRTFNSIMP(void, init, TGEVIRT_ABSTRACTCLASSREP_INIT);

	public:
		typedef bool(*SetDataNotify)(void* obj, const char* data);
		typedef const char* (*GetDataNotify)(void* obj, const char* data);

		class TypeValidator;

		enum ACRFieldTypes
		{
			TypeS8 = 0,
			TypeS32 = 1,
			TypeS32Vector = 2,
			TypeBool = 3,
			TypeBoolVector = 4,
			TypeF32 = 5,
			TypeF32Vector = 6,
			TypeString = 7,
			TypeCaseString = 8,
			TypeFilename = 9,
			TypeEnum = 10,
			TypeFlag = 11,
			TypeColorI = 12,
			TypeColorF = 13,
			TypeSimObjectPtr = 14,
			TypePoint2F = 15,
			//Etc. There are like 50 of these

			StartGroupFieldType = 0xFFFFFFFD,
			EndGroupFieldType = 0xFFFFFFFE,
			DepricatedFieldType = 0xFFFFFFFF
		};

		struct Field {
			const char* pFieldname;    ///< Name of the field.

			U32            type;          ///< A type ID. @see ACRFieldTypes
			U32            offset;        ///< Memory offset from beginning of class for this field.
			S32            elementCount;  ///< Number of elements, if this is an array.
			EnumTable* table;         ///< If this is an enum, this points to the table defining it.
			BitSet32       flag;          ///< Stores various flags
			void* unused;        /// IDK
		};

		typedef Vector<Field> FieldList;
		GETTERFNSIMP(const char*, getClassName, TGEOFF_ABSTRACTCLASSREP_CLASSNAME);
		GETTERFNSIMP(FieldList, getFieldList, TGEOFF_ABSTRACTCLASSREP_FIELDLIST);

	};

	template <class T>
	class ConcreteClassRep : public AbstractClassRep
	{

	};

	class ConsoleObject
	{

	public:
		VTABLE(TGEOFF_CONSOLEOBJECT_VTABLE);

		VIRTFNSIMP(AbstractClassRep*, getClassRep, TGEVIRT_CONSOLEOBJECT_GETCLASSREP);
		VIRTDTOR(~ConsoleObject, TGEVIRT_CONSOLEOBJECT_DESTRUCTOR);

		MEMBERFN(void, addDepricatedField, (const char* fieldname), (fieldname), TGEADDR_CONSOLEOBJECT_ADDDEPRICATEDFIELD);
		MEMBERFN(void, addField, (const char* in_pFieldname, const U32 in_fieldType, const U32 in_fieldOffset, const U32 in_elementCount, EnumTable* in_table), (in_pFieldname, in_fieldType, in_fieldOffset, in_elementCount, in_table), TGEADDR_CONSOLEOBJECT_ADDFIELD);
	};

	class SimObjectList : public VectorPtr<SimObject*>
	{
		static S32 QSORT_CALLBACK compareId(const void* a, const void* b);
	public:
		void pushBack(SimObject*);
		void pushBackForce(SimObject*);
		void pushFront(SimObject*);
		void remove(SimObject*);

		SimObject* at(S32 index) const {
			if (index >= 0 && index < size())
				return (*this)[index];
			return NULL;
		}
		void removeStable(SimObject* pObject);
		void sortId();
	};
	class SimFieldDictionary
	{
	public:
		struct Entry
		{
			const char* slotName;
			char* value;
			Entry* next;
		};

		enum
		{
			HashTableSize = 19
		};
		Entry* mHashTable[HashTableSize];
	};
	namespace Namespace {
		class Namespace;
	}
	enum TypeMasks {
		StaticObjectType = 1 << 0, // 1
		EnvironmentObjectType = 1 << 1, // 2
		TerrainObjectType = 1 << 2, // 4
		InteriorObjectType = 1 << 3, // 8
		WaterObjectType = 1 << 4, // 16
		TriggerObjectType = 1 << 5, // 32
		MarkerObjectType = 1 << 6, // 64
		ForceFieldObjectType = 1 << 8, // 256
		GameBaseObjectType = 1 << 10, // 1024
		ShapeBaseObjectType = 1 << 11, // 2048
		CameraObjectType = 1 << 12, // 4096
		StaticShapeObjectType = 1 << 13, // 8192
		PlayerObjectType = 1 << 14, // 16384
		ItemObjectType = 1 << 15, // 32768
		VehicleObjectType = 1 << 16, // 65536
		VehicleBlockerObjectType = 1 << 17, // 131072
		ProjectileObjectType = 1 << 18, // 262144
		ExplosionObjectType = 1 << 19, // 524288
		CorpseObjectType = 1 << 20, // 1048576
		DebrisObjectType = 1 << 22, // 4194304
		PhysicalZoneObjectType = 1 << 23, // 8388608
		StaticTSObjectType = 1 << 24, // 16777216
		GuiControlObjectType = 1 << 25, // 33554432
		StaticRenderedObjectType = 1 << 26, // 67108864
		DamagableItemObjectType = 1 << 27, // 134217728
	};
	class Stream;
	class SimObject : public ConsoleObject
	{
	public:
		GETTERFNSIMP(const char*, getName, TGEOFF_SIMOBJECT_NAME);
		GETTERFNSIMP(SimObjectId, getId, TGEOFF_SIMOBJECT_ID);
		GETTERFNSIMP(SimObjectId, getType, TGEOFF_SIMOBJECT_TYPE);
		GETTERFNSIMP(Namespace::Namespace*, getNamespace, TGEOFF_SIMOBJECT_NAMESPACE);
		GETTERFNSIMP(SimFieldDictionary*, getFieldDictionary, TGEOFF_SIMOBJECT_FIELDDICTIONARY);
		MEMBERFNSIMP(const char*, getIdString, TGEADDR_SIMOBJECT_GETIDSTRING);
		MEMBERFN(void, setHidden, (bool hidden), (hidden), TGEADDR_SIMOBJECT_SETHIDDEN);
		MEMBERFN(const char*, getDataField, (const char* slotName, const char* array), (slotName, array), TGEADDR_SIMOBJECT_GETDATAFIELD);
		MEMBERFN(void, setDataField, (const char* slotName, const char* array, const char* value), (slotName, array, value), TGEADDR_SIMOBJECT_SETDATAFIELD);

		VIRTFN(bool, processArguments, (S32 argc, const char** argv), (argc, argv), TGEVIRT_SIMOBJECT_PROCESSARGUMENTS);
		VIRTFNSIMP(bool, onAdd, TGEVIRT_SIMOBJECT_ONADD);
		VIRTFNSIMP(bool, onRemove, TGEVIRT_SIMOBJECT_ONREMOVE);
		VIRTFNSIMP(bool, onGroupAdd, TGEVIRT_SIMOBJECT_ONGROUPADD);
		VIRTFNSIMP(bool, onGroupRemove, TGEVIRT_SIMOBJECT_ONGROUPREMOVE);
		VIRTFN(bool, onNameChange, (const char* name), (name), TGEVIRT_SIMOBJECT_ONNAMECHANGE);
		VIRTFN(bool, onStaticModified, (const char* slotName), (slotName), TGEVIRT_SIMOBJECT_ONSTATICMODIFIED);
		VIRTFNSIMP(void, inspectPreApply, TGEVIRT_SIMOBJECT_INSPECTPREAPPLY);
		VIRTFNSIMP(void, inspectPostApply, TGEVIRT_SIMOBJECT_INSPECTPOSTAPPLY);
		VIRTFNSIMP(void, onVideoKill, TGEVIRT_SIMOBJECT_ONVIDEOKILL);
		VIRTFNSIMP(void, onVideoRessurect, TGEVIRT_SIMOBJECT_ONVIDEORESSURECT);
		VIRTFN(void, onDeleteNotify, (SimObject* object), (object), TGEVIRT_SIMOBJECT_ONDELETENOTIFY);
		VIRTFNSIMP(void, onEditorEnable, TGEVIRT_SIMOBJECT_ONEDITORENABLE);
		VIRTFNSIMP(void, onEditorDisable, TGEVIRT_SIMOBJECT_ONEDITORDISABLE);
		VIRTFN(const char*, getEditorClassName, (const char* name), (name), TGEVIRT_SIMOBJECT_GETEDITORCLASSNAME);
		VIRTFN(SimObject*, findObject, (const char* name), (name), TGEVIRT_SIMOBJECT_FINDOBJECT);
		VIRTFN(void, write, (Stream& stream, U32 tabStop, U32 flags), (stream, tabStop, flags), TGEVIRT_SIMOBJECT_WRITE);
		VIRTFN(void, registerLights, (void* lm, bool lightingScene), (lm, lightingScene), TGEVIRT_SIMOBJECT_REGISTERLIGHTS);
		//UNDEFVIRT(onVideoKill);
		//UNDEFVIRT(onVideoResurrect);

		//UNDEFVIRT(getEditorClassName);


		MEMBERFNSIMP(bool, registerObject, TGEADDR_SIMOBJECT_REGISTEROBJECT);
		MEMBERFNSIMP(void, deleteObject, TGEADDR_SIMOBJECT_DELETEOBJECT);

		void dump()
		{
			FILE* f;
			f = fopen("marbledata.bin", "wb");
			fwrite(reinterpret_cast<char*>(this), sizeof(char), 5000, f);
		}
	};

	class NetObject : public SimObject
	{
	public:
		enum NetFlags
		{
			IsGhost = 1 << 1,   ///< This is a ghost.
			ScopeAlways = 6 << 1,  ///< Object always ghosts to clients.
			ScopeLocal = 7 << 1,  ///< Ghost only to local client.
			Ghostable = 8 << 1,  ///< Set if this object CAN ghost.

			MaxNetFlagBit = 15
		};

		VIRTFN(F32, getUpdatePriority, (void* focusObject, U32 updateMask, S32 updateSkips), (focusObject, updateMask, updateSkips), TGEVIRT_NETOBJECT_GETUPDATEPRIORITY);
		MEMBERFN(U32, packUpdate, (NetConnection* connection, U32 mask, BitStream* stream), (connection, mask, stream), TGEADDR_NETOBJECT_PACKUPDATE);
		MEMBERFN(void, unpackUpdate, (NetConnection* connection, BitStream* stream), (connection, stream), TGEADDR_NETOBJECT_UNPACKUPDATE);
		VIRTFN(void, onCameraScopeQuery, (NetConnection* cr, void* camInfo), (cr, camInfo), TGEVIRT_NETOBJECT_ONCAMERASCOPEQUERY);
		//UNDEFVIRT(onCameraScopeQuery);

		MEMBERFNSIMP(bool, onAdd, TGEADDR_NETOBJECT_ONADD);
		MEMBERFN(void, setMaskBits, (U32 bits), (bits), TGEADDR_NETOBJECT_SETMASKBITS);

		MEMBERFNSIMP(void, clearScopeAlways, TGEADDR_NETOBJECT_CLEARSCOPEALWAYS);

		GETTERFNSIMP(U32, getNetFlags, TGEOFF_NETOBJECT_NETFLAGS);
		SETTERFN(U32, setNetFlags, TGEOFF_NETOBJECT_NETFLAGS);

		inline bool isServerObject() { return (getNetFlags() & 2) == 0; }
		inline bool isClientObject() { return (getNetFlags() & 2) != 0; }
	};

	class SceneGraph;
	class SceneState
	{
	public:
	};

	class SceneRenderImage
	{
	public:
	};

	class SceneObject : public NetObject
	{
	public:
		VIRTFNSIMP(void, disableCollision, TGEVIRT_SCENEOBJECT_DISABLECOLLISION);
		VIRTFNSIMP(void, enableCollision, TGEVIRT_SCENEOBJECT_ENABLECOLLISION);
		VIRTFNSIMP(bool, isDisplacable, TGEVIRT_SCENEOBJECT_ISDISPLACABLE);
		VIRTFNSIMP(Point3F, getMomentum, TGEVIRT_SCENEOBJECT_GETMOMENTUM);
		VIRTFN(void, setMomentum, (const Point3F& momentum), (momentum), TGEVIRT_SCENEOBJECT_SETMOMENTUM);
		VIRTFNSIMP(F32, getMass, TGEVIRT_SCENEOBJECT_GETMASS);
		VIRTFN(bool, displaceObject, (const Point3F& displaceVector), (displaceVector), TGEVIRT_SCENEOBJECT_DISPLACEOBJECT);
		VIRTFN(void, setTransformVirt, (const MatrixF& transform), (transform), TGEVIRT_SCENEOBJECT_SETTRANSFORM);
		VIRTFN(void, setScaleVirt, (const VectorF& scale), (scale), TGEVIRT_SCENEOBJECT_SETSCALE);
		VIRTFN(void, setRenderTransformVirt, (const MatrixF& transform), (transform), TGEVIRT_SCENEOBJECT_SETRENDERTRANSFORM);
		VIRTFN(void, buildConvex, (const Box3F& box, char* convex), (box, convex), TGEVIRT_SCENEOBJECT_BUILDCONVEX);
		VIRTFN(void, buildPolyList, (void* polyList, const Box3F& box, const SphereF& sphere), (polyList, box, sphere), TGEVIRT_SCENEOBJECT_BUILDPOLYLIST);
		UNDEFVIRT(buildCollisionBSP);
		VIRTFN(bool, castRay, (const Point3F& start, const Point3F& end, RayInfo* info), (start, end, info), TGEVIRT_SCENEOBJECT_CASTRAY);
		VIRTFN(bool, collideBox, (const Point3F& start, const Point3F& end, RayInfo* info), (start, end, info), TGEVIRT_SCENEOBJECT_COLLIDEBOX);
		VIRTFN(bool, getOverlappingZones, (SceneObject* obj, U32* zones, U32* numZones), (obj, zones, numZones), TGEVIRT_SCENEOBJECT_GETOVERLAPPINGZONES);
		VIRTFN(U32, getPointZone, (const Point3F& p), (p), TGEVIRT_SCENEOBJECT_GETPOINTZONE);
		VIRTFN(void, renderObject, (SceneState* state, SceneRenderImage* renderImage), (state, renderImage), TGEVIRT_SCENEOBJECT_RENDEROBJECT);
		VIRTFN(void, renderShadowVolumes, (SceneState* state, SceneRenderImage* renderImage), (state, renderImage), TGEVIRT_SCENEOBJECT_RENDERSHADOWVOLUMES);
		VIRTFN(bool, prepRenderImage, (SceneState* state, const U32 stateKey, const U32 startZone, const bool modifyBaseZoneState), (state, stateKey, startZone, modifyBaseZoneState), TGEVIRT_SCENEOBJECT_PREPRENDERIMAGE);
		VIRTFN(U32, scopeObject, (const Point3F& rootPosition, const F32 rootDistance, bool* zoneScopeState), (rootPosition, rootDistance, zoneScopeState), TGEVIRT_SCENEOBJECT_SCOPEOBJECT);
		VIRTFN(void*, getMaterialProperty, (U32 index), (index), TGEVIRT_SCENEOBJECT_GETMATERIALPROPERTY);
		//UNDEFVIRT(getMaterialProperty);
		VIRTFN(bool, onSceneAdd, (SceneGraph* graph), (graph), TGEVIRT_SCENEOBJECT_ONSCENEADD);
		VIRTFNSIMP(bool, onSceneRemove, TGEVIRT_SCENEOBJECT_ONSCENEREMOVE);
		VIRTFN(void, transformModelview, (const U32 portalIndex, const MatrixF& oldMV, MatrixF* newMV), (portalIndex, oldMV, newMV), TGEVIRT_SCENEOBJECT_TRANSFORMMODELVIEW);
		VIRTFN(void, transformPosition, (const U32 portalIndex, Point3F* point), (portalIndex, point), TGEVIRT_SCENEOBJECT_TRANSFORMPOSITION);
		UNDEFVIRT(computeNewFrustum);
		UNDEFVIRT(openPortal);
		UNDEFVIRT(closePortal);
		UNDEFVIRT(getWSPortalPlane);
		VIRTFNSIMP(bool, installLights, TGEVIRT_SCENEOBJECT_INSTALLLIGHTS);
		VIRTFNSIMP(bool, uninstallLights, TGEVIRT_SCENEOBJECT_UNINSTALLLIGHTS);
		//UNDEFVIRT(installLights);
		//UNDEFVIRT(uninstallLights);
		UNDEFVIRT(getLightingAmbientColor);

		MEMBERFN(void, setTransform, (const MatrixF& transform), (transform), TGEADDR_SCENEOBJECT_SETTRANSFORM);
		MEMBERFN(void, setRenderTransform, (const MatrixF& transform), (transform), TGEADDR_SCENEOBJECT_SETRENDERTRANSFORM);

		//		MEMBERFN(void, renderObject, (void *sceneState, void *sceneRenderImage), (sceneState, sceneRenderImage), TGEADDR_SHAPEBASE_RENDEROBJECT);

		GETTERFN(const MatrixF&, MatrixF, getTransform, TGEOFF_SCENEOBJECT_TRANSFORM);
		GETTERFN(const Box3F&, Box3F, getWorldBox, TGEOFF_SCENEOBJECT_WORLDBOX);

		SETTERFN(MatrixF, setTransformMember, TGEOFF_SCENEOBJECT_TRANSFORM);
		SETTERFN(Box3F, setWorldBox, TGEOFF_SCENEOBJECT_WORLDBOX);

		GETTERFNSIMP(Point3F, getScale, TGEOFF_SCENEOBJECT_SCALE);
		MEMBERFN(void, setScale, (const VectorF& scale), (scale), TGEADDR_SHAPEBASE_SETSCALE);
	};

	struct Collision
	{

		SceneObject* object;
		Point3F point;
		VectorF normal;
		void* material;

		// Face and Face dot are currently only set by the extrudedPolyList
		// clipper.  Values are otherwise undefined.
		U32 face;                  // Which face was hit
		F32 faceDot;               // -Dot of face with poly normal
		F32 distance;

		Collision() :
			object(NULL),
			material(NULL)
		{
			face = 0;
			faceDot = 0.0f;
			distance = 0.0f;
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

		RayInfo() : t(0.0f) {}
	};

	class SimSet : public SimObject
	{
		char unknown[0x2c];
	public:
		SimObjectList objectList;
		GETTERFNSIMP(int, getCount, 48);
		VIRTFN(bool, reOrder, (SimObject* obj, SimObject* target), (obj, target), TGEVIRT_SIMSET_REORDER);

		VIRTFN(void, addObject, (SimObject* obj), (obj), TGEVIRT_SIMSET_ADDOBJECT);
		VIRTFN(void, removeObject, (SimObject* obj), (obj), TGEVIRT_SIMSET_REMOVEOBJECT);
		VIRTFN(void, pushObject, (SimObject* obj), (obj), TGEVIRT_SIMSET_PUSHOBJECT);
		VIRTFNSIMP(void, addObject, TGEVIRT_SIMSET_POPOBJECT);
	};
	class SimNameDictionary
	{
		SimObject** hashTable;
		S32 hashTableSize;
		S32 hashEntryCount;

		void* mutex;
	public:
		void insert(SimObject* obj);
		void remove(SimObject* obj);
		SimObject* find(const char* name);

		SimNameDictionary();
		~SimNameDictionary();
	};
	class SimGroup : public SimSet
	{
		SimNameDictionary nameDictionary;
	};

	class SimDataBlock : public SimObject
	{
	public:
		//UNDEFVIRT(onStaticModified);
		VIRTFN(void, packData_virt, (BitStream* stream), (stream), TGEVIRT_SIMDATABLOCK_PACKDATA);
		VIRTFN(void, unpackData, (BitStream* stream), (stream), TGEVIRT_SIMDATABLOCK_UNPACKDATA);
		VIRTFN(bool, preload, (bool server, char errorbuffer[256]), (server, errorbuffer), TGEVIRT_SIMDATABLOCK_PRELOAD);

		MEMBERFN(void, packData, (BitStream* stream), (stream), TGEADDR_SIMDATABLOCK_PACKDATA);
	};

	class AudioStreamSource
	{
		VTABLE(0);
	public:
		VIRTDTOR(AudioStreamSource, TGEVIRT_AUDIOSTREAMSOURCE_DESTRUCTOR);
		VIRTFNSIMP(bool, initStream, TGEVIRT_AUDIOSTREAMSOURCE_INITSTREAM);
		VIRTFNSIMP(bool, updateBuffers, TGEVIRT_AUDIOSTREAMSOURCE_UPDATEBUFFERS);
		VIRTFNSIMP(void, freestream, TGEVIRT_AUDIOSTREAMSOURCE_FREESTREAM);
	};

	class WavStreamSource : public AudioStreamSource
	{

	};

	class VorbisStreamSource : public AudioStreamSource
	{

	};

	class SimDataBlock;
	class AudioProfile : public SimDataBlock
	{

	};

	class AbstractPolyList
	{
		VTABLE(0);
	public:
		VIRTDTOR(~AbstractPolyList, TGEVIRT_ABSTRACTPOLYLIST_DESTRUCTOR);
		VIRTFNSIMP(bool, isEmpty, TGEVIRT_ABSTRACTPOLYLIST_ISEMPTY);
		VIRTFN(U32, addPoint, (const Point3F& point), (point), TGEVIRT_ABSTRACTPOLYLIST_ADDPOINT);
		VIRTFN(U32, addPlane, (const PlaneF& plane), (plane), TGEVIRT_ABSTRACTPOLYLIST_ADDPLANE);
		VIRTFN(void, begin, (U32 material, U32 surfaceKey), (material, surfaceKey), TGEVIRT_ABSTRACTPOLYLIST_BEGIN);
		VIRTFN(void, plane_v1v2v3, (U32 v1, U32 v2, U32 v3), (v1, v2, v3), TGEVIRT_ABSTRACTPOLYLIST_PLANE_INT_INT_INT);
		VIRTFN(void, plane_planeF, (const PlaneF& plane), (plane), TGEVIRT_ABSTRACTPOLYLIST_PLANE_PLANEF);
		VIRTFN(void, plane_int, (U32 index), (index), TGEVIRT_ABSTRACTPOLYLIST_PLANE_INT);
		VIRTFN(void, vertex, (U32 index), (index), TGEVIRT_ABSTRACTPOLYLIST_VERTEX);
		VIRTFNSIMP(void, end, TGEVIRT_ABSTRACTPOLYLIST_END);
		VIRTFN(bool, getMapping, (MatrixF* transforms, Box3F* bounds), (transforms, bounds), TGEVIRT_ABSTRACTPOLYLIST_GETMAPPING);
		VIRTFN(bool, isInterestedInPlane_planeF, (const PlaneF& plane), (plane), TGEVIRT_ABSTRACTPOLYLIST_ISINTERESTEDINPLANE_PLANEF);
		VIRTFN(bool, isInterestedInPlane_int, (const U32 index), (index), TGEVIRT_ABSTRACTPOLYLIST_ISINTERESTEDINPLANE_INT);
		VIRTFN(const PlaneF&, getIndexedPlane, (const U32 index), (index), TGEVIRT_ABSTRACTPOLYLIST_GETINDEXEDPLANE);
	};

	class ClippedPolyList : public AbstractPolyList
	{

	};

	class DepthSortList : public ClippedPolyList
	{

	};

	class EarlyOutPolyList : public AbstractPolyList
	{

	};

	class ExtrudedPolyList : public AbstractPolyList
	{

	};

	class PlaneExtractorPolyList : public AbstractPolyList
	{

	};

	class ConcretePolyList : public AbstractPolyList
	{

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
		VIRTFN(bool, _read, (U32 size, void* buf), (size, buf), TGEVIRT_STREAM__READ);
		VIRTFN(bool, _write, (U32 size, const void* buf), (size, buf), TGEVIRT_STREAM__WRITE);
		VIRTFN(bool, hasCapability, (int capability), (capability), TGEVIRT_STREAM_HASCAPABILITY);
		VIRTFNSIMP(U32, getPosition, TGEVIRT_STREAM_GETPOSITION);
		VIRTFN(bool, setPosition, (U32 pos), (pos), TGEVIRT_STREAM_SETPOSITION);
		VIRTFNSIMP(U32, getStreamSize, TGEVIRT_STREAM_GETSTREAMSIZE);
		VIRTFN(void, readString, (char* str), (str), TGEVIRT_STREAM_READSTRING);
		VIRTFN(void, writeString, (const char* str, S32 maxLength), (str, maxLength), TGEVIRT_STREAM_WRITESTRING);
	};

	class FileStream : public Stream
	{
	public:
		MEMBERFN(bool, open, (const char* path, int accessMode), (path, accessMode), TGEADDR_FILESTREAM_OPEN);
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

		MEMBERFN(FileStatus, open, (const char* filename, const AccessMode openMode), (filename, openMode), TGEADDR_FILE_OPEN);
		MEMBERFNSIMP(U32, getPosition, TGEADDR_FILE_GETPOSITION);
		MEMBERFN(FileStatus, setPosition, (S32 position, bool absolutePos), (position, absolutePos), TGEADDR_FILE_SETPOSITION);
		MEMBERFNSIMP(U32, getSize, TGEADDR_FILE_GETSIZE);
		MEMBERFNSIMP(FileStatus, flush, TGEADDR_FILE_FLUSH);
		MEMBERFNSIMP(FileStatus, close, TGEADDR_FILE_CLOSE);
		MEMBERFNSIMP(FileStatus, getStatus, TGEADDR_FILE_GETSTATUS);
		MEMBERFN(FileStatus, read, (U32 size, char* dst, U32* bytesRead), (size, dst, bytesRead), TGEADDR_FILE_READ);
		MEMBERFN(FileStatus, write, (U32 size, const char* src, U32* bytesWritten), (size, src, bytesWritten), TGEADDR_FILE_WRITE);
	};

	class FileObject : public SimObject
	{
	public:
		FileStream* getStream() {
			//Weird method of getting the stream because it's actually not a pointer,
			// but we can't make a reference to it because FileStream is abstract. So,
			// this works.
			return reinterpret_cast<FileStream*>(reinterpret_cast<char*>(this) + TGEOFF_FILEOBJECT_FILESTREAM);
		}

		bool _read(U32 size, void* dst, U32* bytesRead = NULL) {
			//Torque just caches all of this in ram. Bad Torque!
			U32 position = getCurPos();

			//At least it makes this easy
			if (size + position > getBufferSize()) {
				size = getBufferSize() - position;
			}

			//Ugh, copy from memory to memory
			memcpy(dst, getFileBuffer() + position, size);

			//*audible groan*
			setCurPos(position + size);
			if (bytesRead != NULL)
				*bytesRead = size;
			return true;
		}
		bool _write(U32 size, const void* src) {
			return getStream()->_write(size, src);
		}


		GETTERFNSIMP(U8*, getFileBuffer, TGEOFF_FILEOBJECT_FILEBUFFER);
		GETTERFNSIMP(U32, getBufferSize, TGEOFF_FILEOBJECT_BUFFERSIZE);
		GETTERFNSIMP(U32, getCurPos, TGEOFF_FILEOBJECT_CURPOS);
		SETTERFN(U32, setCurPos, TGEOFF_FILEOBJECT_CURPOS);
	};

	class BitStream : public Stream
	{
	public:
		MEMBERFN(void, writeInt, (S32 value, S32 bitCount), (value, bitCount), TGEADDR_BITSTREAM_WRITEINT);
		MEMBERFN(S32, readInt, (S32 bitCount), (bitCount), TGEADDR_BITSTREAM_READINT);

		bool writeFlag(bool flag) {
			writeInt(flag, 8);
			return flag;
		}
		bool readFlag() {
			return readInt(8) != 0;
		}
	};

	class ResizeBitStream : public BitStream
	{

	};

	class CameraFX
	{
		VTABLE(0);
	public:
		VIRTFN(void, update, (U32 delta), (delta), 0);

	};

	class CameraShake : public CameraFX
	{
	public:
		MEMBERFNSIMP(void, init, 0x04020B3);
	};

	class GameBaseData : public SimDataBlock
	{

	};

	class ShapeBaseData : public GameBaseData
	{
	public:
		GETTERFNSIMP(const char*, getShapeFile, TGEOFF_SHAPEBASEDATA_SHAPEFILE);
	};

	class MarbleData : public ShapeBaseData
	{
	public:
		MEMBERFN(void, packData, (BitStream* stream), (stream), TGEADDR_MARBLEDATA_PACKDATA);
		GETTERFNSIMP(F32, getCollisionRadius, TGEOFF_MARBLEDATA_COLLISION_RADIUS);
		SETTERFN(F32, setCollisionRadius, TGEOFF_MARBLEDATA_COLLISION_RADIUS);

		GETTERFNSIMP(void*, getJumpSound, TGEOFF_MARBLEDATA_JUMPSOUND);
	};

	class ShapeBaseImageData : public GameBaseData
	{

	};

	class GameBase : public SceneObject
	{
	public:
		GETTERFNSIMP(GameConnection*, getControllingClient, TGEOFF_GAMEBASE_CONTROLLINGCLIENT);
		GETTERFNSIMP(GameBaseData*, getDataBlock, TGEOFF_GAMEBASE_DATABLOCK);

		VIRTFN(bool, onNewDataBlock, (GameBaseData* dptr), (dptr), TGEVIRT_GAMEBASE_ONNEWDATABLOCK);
		VIRTFN(void, processTick, (const Move* move), (move), TGEVIRT_GAMEBASE_PROCESSTICK);
		VIRTFN(void, interpolateTick, (F32 delta), (delta), TGEVIRT_GAMEBASE_INTERPOLATETICK);
		VIRTFN(void, advanceTime, (F32 delta), (delta), TGEVIRT_GAMEBASE_ADVANCETIME);
		VIRTFN(void, advancePhysics, (const Move* move, U32 delta), (move, delta), TGEVIRT_GAMEBASE_ADVANCEPHYSICS);
		VIRTFNSIMP(Point3D, getVelocity, TGEVIRT_GAMEBASE_GETVELOCITY);
		VIRTFN(void*, getForce, (Point3F& arg1, Point3F* arg2), (arg1, arg2), TGEVIRT_GAMEBASE_GETFORCE);
		//UNDEFVIRT(getVelocity);
		//UNDEFVIRT(getForce);
		VIRTFN(void, writePacketData, (GameConnection* conn, BitStream* stream), (conn, stream), TGEVIRT_GAMEBASE_WRITEPACKETDATA);
		VIRTFN(void, readPacketData, (GameConnection* conn, BitStream* stream), (conn, stream), TGEVIRT_GAMEBASE_READPACKETDATA);
		VIRTFN(U32, getPacketDataChecksum, (GameConnection* conn), (conn), TGEVIRT_GAMEBASE_GETPACKETDATACHECKSUM);

		MEMBERFNSIMP(void, scriptOnAdd, TGEADDR_GAMEBASE_SCRIPTONADD);
	};

	class Shadow
	{
	public:
		MEMBERFN(void, setRadius, (void* shapeInstance, const Point3F& scale), (shapeInstance, scale), TGEADDR_SHADOW_SETRADIUS);
	};

	class ParticleEmitter : public GameBase
	{
	public:
		MEMBERFNSIMP(bool, onAdd, TGEADDR_PARTICLEEMITTER_ONADD);
		MEMBERFNSIMP(void, onRemove, TGEADDR_PARTICLEEMITTER_ONREMOVE);
		MEMBERFN(void, renderObject, (SceneState* state, SceneRenderImage* image), (state, image), TGEADDR_PARTICLEEMITTER_RENDEROBJECT);
		MEMBERFN(void, emitParticles, (const Point3F& start, const Point3F& end, const Point3F& axis, const Point3F& velocity, const U32 numMilliseconds), (start, end, axis, velocity, numMilliseconds), TGEADDR_PARTICLEEMITTER_EMITPARTICLES);
	};

	struct Particle
	{
		Point3F  pos;     // current instantaneous position
		Point3F  vel;     //   "         "         velocity
		Point3F  acc;     // Constant acceleration
		Point3F  orientDir;  // direction particle should go if using oriented particles

		U32           totalLifetime;   // Total ms that this instance should be "live"
		int dataBlock;       // datablock that contains global parameters for
									   //  this instance

		Particle* nextInList;   // Managed by the current owning emitter
		U32       currentAge;

		Particle* nextInEngine; // Managed by the global engine object
		ParticleEmitter* currentOwner;
		Point4F           color;
		F32              size;
		F32              spinSpeed;
	};

	class PEEngine
	{
	};

	class TSThread
	{
	public:
		GETTERFNSIMP(float, getPos, 0xC);
		SETTERFN(float, setPos, 0xC);
	};

	class TSShapeInstance
	{
	public:
		MEMBERFN(void, setPos, (TSThread* thread, F32 pos), (thread, pos), TGEADDR_TSSHAPEINSTANCE_SETTHREADPOS);
		MEMBERFN(void, setTimeScale, (TSThread* thread, F32 t), (thread, t), TGEADDR_TSSHAPEINSTANCE_SETTIMESCALE);
	};

	struct Thread {
		/// State of the animation thread.
		enum State {
			Play, Stop, Pause, FromMiddle
		};
		TSThread* thread; ///< Pointer to 3space data.
		U32 state;        ///< State of the thread
						  ///
						  ///  @see Thread::State
		S32 sequence;     ///< The animation sequence which is running in this thread.
		U32 sound;        ///< Handle to sound.
		bool atEnd;       ///< Are we at the end of this thread?
		bool forward;     ///< Are we playing the thread forward? (else backwards)
	};

	class ShapeBase : public GameBase
	{
	public:
		MEMBERFNSIMP(bool, isHidden, TGEADDR_SHAPEBASE_ISHIDDEN);
		VIRTFN(void, onCollision, (ShapeBase* object, VectorF vec), (object, vec), TGEVIRT_SHAPEBASE_ONCOLLISION);
		VIRTFN(void, setImage, (U32 imageSlot, ShapeBaseImageData* imageData, int& skinNameHandle, bool loaded, bool ammo, bool triggerDown, bool target), (imageSlot, imageData, skinNameHandle, loaded, ammo, triggerDown, target), TGEVIRT_SHAPEBASE_SETIMAGE);
		//UNDEFVIRT(onDeleteNotify);
		UNDEFVIRT(onImageRecoil);
		UNDEFVIRT(ejectShellCasing);
		UNDEFVIRT(updateDamageLevel);
		UNDEFVIRT(updateDamageState);
		UNDEFVIRT(blowUp);
		UNDEFVIRT(onMount);
		UNDEFVIRT(onUnmount);
		UNDEFVIRT(onImpact_SceneObject_Point3F);
		UNDEFVIRT(onImpact_Point3F);
		//UNDEFVIRT(controlPrePacketSend);
		UNDEFVIRT(setEnergyLevel);
		UNDEFVIRT(mountObject);
		UNDEFVIRT(mountImage);
		UNDEFVIRT(unmountImage);
		UNDEFVIRT(getMuzzleVector);
		UNDEFVIRT(getCameraParameters);
		VIRTFN(void, getCameraTransform, (F32* pos, MatrixF* mat), (pos, mat), TGEVIRT_SHAPEBASE_GETCAMERATRANSFORM);
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
		UNDEFVIRT(renderShadow);
		UNDEFVIRT(calcClassRenderData);
		//VIRTFN(void, setHidden, (bool show), (show), TGEVIRT_SHAPEBASE_SETHIDDEN);
		//UNDEFVIRT(onCollision);
		//UNDEFVIRT(getSurfaceFriction);
		//UNDEFVIRT(getBounceFriction);

		MEMBERFN(void, setHidden, (bool hidden), (hidden), 0x40104B);


		GETTERFNSIMP(F32, getFadeVal, TGEOFF_SHAPEBASE_FADEVAL);
		SETTERFN(F32, setFadeVal, TGEOFF_SHAPEBASE_FADEVAL);

		GETTERFNSIMP(bool, getHiddenGetter, TGEOFF_SHAPEBASE_HIDDEN);
		SETTERFN(bool, setHiddenSetter, TGEOFF_SHAPEBASE_HIDDEN);

		GETTERFNSIMP(TSThread*, getThread1, TGEOFF_SHAPEBASE_THREAD1);
		GETTERFNSIMP(TSThread*, getThread2, TGEOFF_SHAPEBASE_THREAD2);

		GETTERFNSIMP(Thread::State, getThread1State, TGEOFF_SHAPEBASE_THREAD1STATE);
		SETTERFN(Thread::State, setThread1State, TGEOFF_SHAPEBASE_THREAD1STATE);

		GETTERFNSIMP(bool, getThread1Forward, TGEOFF_SHAPEBASE_THREAD1FORWARD);
		SETTERFN(bool, setThread1Forward, TGEOFF_SHAPEBASE_THREAD1FORWARD);

		GETTERFNSIMP(bool, getThread1AtEnd, TGEOFF_SHAPEBASE_THREAD1FORWARD - 1);
		SETTERFN(bool, setThread1AtEnd, TGEOFF_SHAPEBASE_THREAD1FORWARD - 1);

		SETTERFN(TSThread*, setThread1, TGEOFF_SHAPEBASE_THREAD1);
		SETTERFN(TSThread*, setThread2, TGEOFF_SHAPEBASE_THREAD2);

		GETTERFNSIMP(TSShapeInstance*, getTSShapeInstance, TGEOFF_SHAPEBASE_SHAPEINSTANCE);

		MEMBERFN(void, renderObject, (void* sceneState, void* sceneRenderImage), (sceneState, sceneRenderImage), TGEADDR_SHAPEBASE_RENDEROBJECT);

		MEMBERFN(U32, packUpdate, (NetConnection* connection, U32 mask, BitStream* stream), (connection, mask, stream), TGEADDR_SHAPEBASE_PACKUPDATE);
		MEMBERFN(void, unpackUpdate, (NetConnection* connection, BitStream* stream), (connection, stream), TGEADDR_SHAPEBASE_UNPACKUPDATE);

		MEMBERFN(void, updateThread, (Thread& st), (st), TGEADDR_SHAPEBASE_UPDATETHREAD);
	};

	class Item : public ShapeBase
	{

	};

	class Marble : public ShapeBase
	{
	public:
		GETTERFNSIMP(Point3D, getVelocity, TGEOFF_MARBLE_VELOCITY);
		SETTERFN(Point3D, setVelocity, TGEOFF_MARBLE_VELOCITY);
		GETTERFNSIMP(Point3D, getAngularVelocity, TGEOFF_MARBLE_ANGULARVELOCITY);
		SETTERFN(Point3D, setAngularVelocity, TGEOFF_MARBLE_ANGULARVELOCITY);

		GETTERFNSIMP(F32, getCameraYaw, TGEOFF_MARBLE_CAMERAYAW);
		SETTERFN(F32, setCameraYaw, TGEOFF_MARBLE_CAMERAYAW);
		GETTERFNSIMP(F32, getCameraPitch, TGEOFF_MARBLE_CAMERAPITCH);
		SETTERFN(F32, setCameraPitch, TGEOFF_MARBLE_CAMERAPITCH);

		GETTERFNSIMP(Box3F, getCollisionBox, TGEOFF_MARBLE_COLLISION_BOX);
		SETTERFN(Box3F, setCollisionBox, TGEOFF_MARBLE_COLLISION_BOX);
		GETTERFNSIMP(F32, getCollisionRadius, TGEOFF_MARBLE_COLLISION_RADIUS);
		SETTERFN(F32, setCollisionRadius, TGEOFF_MARBLE_COLLISION_RADIUS);

		GETTERFNSIMP(bool, getOOB, TGEOFF_MARBLE_OOB);
		SETTERFN(bool, setOOB, TGEOFF_MARBLE_OOB);

		GETTERFNSIMP(bool, getControllable, TGEOFF_MARBLE_CONTROLLABLE);
		SETTERFN(bool, setControllable, TGEOFF_MARBLE_CONTROLLABLE);

		MEMBERFN(U32, packUpdate, (TGE::NetConnection* connection, U32 mask, TGE::BitStream* stream), (connection, mask, stream), TGEADDR_MARBLE_PACKUPDATE);
		MEMBERFN(void, unpackUpdate, (TGE::NetConnection* connection, TGE::BitStream* stream), (connection, stream), TGEADDR_MARBLE_UNPACKUPDATE);

		MEMBERFN(void, setPosition, (const Point3D& position, const AngAxisF& camera, F32 pitch), (position, camera, pitch), TGEADDR_MARBLE_SETPOSITION);
		MEMBERFN(void, setPositionSimple, (const Point3D& position), (position), TGEADDR_MARBLE_SETPOSITION_SIMPLE);

		MEMBERFN(void, doPowerUp, (S32 powerUpId), (powerUpId), TGEADDR_MARBLE_DOPOWERUP);

		MEMBERFNSIMP(bool, onAdd, TGEADDR_MARBLE_ONADD);
		MEMBERFN(void, setTransform, (const MatrixF& mat), (mat), TGEADDR_MARBLE_SETTRANSFORM);

	};

	class Trigger : public GameBase
	{
	public:
		MEMBERFNSIMP(bool, onAdd, TGEADDR_TRIGGER_ONADD);
		MEMBERFNSIMP(void, onEditorEnable, TGEADDR_TRIGGER_ONEDITORENABLE);
		MEMBERFNSIMP(void, onEditorDisable, TGEADDR_TRIGGER_ONEDITORDISABLE);
		MEMBERFN(bool, testObject, (GameBase* enter), (enter), TGEADDR_TRIGGER_TESTOBJECT);
		MEMBERFN(void, renderObject, (void* sceneState, void* sceneRenderImage), (sceneState, sceneRenderImage), TGEADDR_TRIGGER_RENDEROBJECT);
		MEMBERFN(U32, packUpdate, (NetConnection* connection, U32 mask, BitStream* stream), (connection, mask, stream), TGEADDR_TRIGGER_PACKUPDATE);
		MEMBERFN(void, unpackUpdate, (NetConnection* connection, BitStream* stream), (connection, stream), TGEADDR_TRIGGER_UNPACKUPDATE);
	};

	class GuiControlProfile;
	class GuiControl : public SimGroup
	{
	public:
		GETTERFNSIMP(bool, isAwake, TGEOFF_GUICONTROL_ISAWAKE);
		GETTERFNSIMP(RectI, getBounds, TGEOFF_GUICONTROL_BOUNDS);
		GETTERFNSIMP(Point2I, getPosition, TGEOFF_GUICONTROL_POSITION);
		GETTERFNSIMP(Point2I, getExtent, TGEOFF_GUICONTROL_EXTENT);
		GETTERFNSIMP(GuiControlProfile*, getProfile, TGEOFF_GUICONTROL_MPROFILE);
		GETTERFNSIMP(bool, getActive, TGEOFF_GUICONTROL_ACTIVE);
		GETTERFNSIMP(bool, getDepressed, TGEOFF_GUICONTROL_DEPRESSED);
		GETTERFNSIMP(bool, getMouseOver, TGEOFF_GUICONTROL_MOUSEOVER);
		GETTERFNSIMP(bool, getStateOn, TGEOFF_GUICONTROL_STATEON);
		SETTERFN(Point2I, setExtent, TGEOFF_GUICONTROL_EXTENT);
		MEMBERFN(void, renderChildControls, (Point2I offset, RectI const& rect), (offset, rect), TGEADDR_GUICONTROL_RENDERCHILDCONTROLS);
		MEMBERFN(void, renderJustifiedText, (Point2I offset, Point2I extent, const char* text), (offset, extent, text), TGEADDR_GUICONTROL_RENDERJUSTIFIEDTEXT);
		MEMBERFN(void, onRender, (Point2I offset, const RectI& rect), (offset, rect), TGEADDR_GUICONTROL_ONRENDER);
	};

	class GuiMLTextCtrl : public GuiControl
	{
	public:
		GETTERFNSIMP(U32, getCursorPosition, 0x13C);
		GETTERFNSIMP(U32, setCursorPosition, 0x13C);
	};

	class GuiMLTextEditCtrl : public GuiMLTextCtrl
	{

	};

	class GuiCanvas : public GuiControl
	{
		MEMBERFN(void, renderFrame, (bool bufferSwap), (bufferSwap), TGEADDR_CANVAS_RENDERFRAME);
	};
	class GuiWindowCtrl : public GuiControl
	{
	public:
		MEMBERFN(void, resize, (Point2I const& pos, Point2I const& extent), (pos, extent), TGEADDR_GUIWINDOWCTRL_RESIZE);
	};
	class TextureHandle;
	class GuiBitmapCtrl : public GuiControl
	{
	public:
		GETTERFNSIMP(TextureHandle*, getTextureHandle, TGEOFF_GUIBITMAPCTRL_TEXTUREHANDLE);
		GETTERFNSIMP(Point2I, getStartPoint, TGEOFF_GUIBITMAPCTRL_STARTPOINT);
		GETTERFNSIMP(bool, getWrap, TGEOFF_GUIBITMAPCTRL_WRAP);
		GETTERFNSIMP(const char*, getBitmap, TGEOFF_GUIBITMAPCTRL_BITMAP);
		MEMBERFN(void, onRender, (Point2I offset, const RectI& updateRect), (offset, updateRect), TGEADDR_GUIBITMAPCTRL_ONRENDER);
	};
	class GuiBorderButtonCtrl : public GuiControl
	{
	public:
		GETTERFNSIMP(const char*, getButtonText, TGEOFF_GUIBUTTONCTRL_BUTTONTEXT);
		MEMBERFN(void, onRender, (Point2I offset, const RectI& updateRect), (offset, updateRect), TGEADDR_GUIBORDERBUTTONCTRL_ONRENDER);
	};

	struct Resolution
	{
		Point2I size;
		U32 bpp;
	};

	class DisplayDevice
	{
	public:
		VTABLE(0);
		UNDEFVIRT(initDevice);
		VIRTFN(bool, activate, (U32 width, U32 height, U32 bpp, bool fullScreen), (width, height, bpp, fullScreen), 1);
		VIRTFNSIMP(void, shutdown, 2);
		VIRTFN(bool, setScreenMode, (U32 width, U32 height, U32 bpp, bool fullScreen, bool forceIt, bool repaint), (width, height, bpp, fullScreen, forceIt, repaint), 3);
		UNDEFVIRT(setResolution);
		UNDEFVIRT(toggleFullScreen);
		UNDEFVIRT(swapBuffers);
		UNDEFVIRT(getDriverInfo);
		UNDEFVIRT(getGammaCorrection);
		UNDEFVIRT(setGammaCorrection);
		UNDEFVIRT(setVerticalSync);
	};

	GLOBALVAR(bool, isFullScreen, TGEADDR_DISPLAYDEVICE_ISFULLSCREEN);

	class OpenGLDevice : public DisplayDevice
	{
	public:
		MEMBERFN(bool, activate, (U32 width, U32 height, U32 bpp, bool fullScreen), (width, height, bpp, fullScreen), 0x4033B9);
		MEMBERFN(void, shutdown, (bool force), (force), 0x40914C);
		MEMBERFN(bool, setScreenMode, (U32 width, U32 height, U32 bpp, bool fullScreen, bool forceIt, bool repaint), (width, height, bpp, fullScreen, forceIt, repaint), 0x406366);
	};

	typedef int NetConnectionId;

	class NetEvent : public ConsoleObject
	{
	};

	class ConnectionProtocol
	{
		VTABLE(0);
	public:
		VIRTFN(void, writeDemoStartBlock, (BitStream* stream), (stream), 0);
		VIRTFN(bool, readDemoStartBlock, (BitStream* stream), (stream), 1);
		VIRTFN(void, processRawPacket, (BitStream* stream), (stream), 2);
	};

	GLOBALVAR(TGE::SimObject*, mServerConnection, TGEADDR_MSERVERCONNECTION);

	class NetConnection : public ConnectionProtocol, public SimObject
	{
	public:
		static NetConnection* getConnectionToServer()
		{
			return static_cast<TGE::NetConnection*>(mServerConnection);
		}
		GETTERFNSIMP(F32, getPing, TGEOFF_NETCONNECTION_PING);
		MEMBERFN(bool, postNetEvent, (NetEvent* event), (event), TGEADDR_NETCONNECTION_POSTNETEVENT);
		MEMBERFN(S32, getGhostIndex, (NetObject* obj), (obj), TGEADDR_NETCONNECTION_GETGHOSTINDEX);
		MEMBERFN(NetObject*, resolveGhost, (S32 id), (id), TGEADDR_NETCONNECTION_RESOLVEGHOST);
	};

	class Sky : public NetObject
	{
	public:
		MEMBERFNSIMP(bool, onAdd, TGEADDR_SKY_ONADD);
		MEMBERFN(U32, packUpdate, (NetConnection* connection, U32 mask, BitStream* stream), (connection, mask, stream), TGEADDR_SKY_PACKUPDATE);
		MEMBERFN(void, unpackUpdate, (NetConnection* connection, BitStream* stream), (connection, stream), TGEADDR_SKY_UNPACKUPDATE);

		GETTERFNSIMP(const char*, getMaterialList, TGEOFF_SKY_MATERIALLIST);
	};

	class Sun : public NetObject
	{
	public:
		MEMBERFNSIMP(bool, onAdd, TGEADDR_SUN_ONADD);
		GETTERFNSIMP(Point3F, getDirection, TGEOFF_SUN_DIRECTION);
	};

#ifdef __APPLE__
	class TCPObject: public SimObject
	{
	public:
		MEMBERFNSIMP(void, onAdd, TGEADDR_TCPOBJECT_ONADD);
		MEMBERFNSIMP(void, ctor, TGEADDR_TCPOBJECT_CTOR);
		MEMBERFNSIMP(void, dtor, TGEADDR_TCPOBJECT_DTOR);
		MEMBERFN(void, connect, (U32 argc, const char **argv), (argc, argv), TGEADDR_TCPOBJECT_CONNECT);
		MEMBERFN(void, send, (U32 argc, const char **argv), (argc, argv), TGEADDR_TCPOBJECT_SEND);
	};
#endif // __APPLE___


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

	
	class Camera: public ShapeBase
	{
		MEMBERFN(void, advancePhysics, (const Move *move, U32 delta), (move, delta), TGEADDR_CAMERA_ADVANCEPHYSICS);
	};

   struct ItrFastDetail
   {
   public:
      struct VertexData {
         Point3F vertex;     // vertex    at  0
         Point3F normal;     // normal    at 12
         Point2F texCoord;   // tex coord at 24
         int windingIndex;
         int texNum; // maybe
      };
      struct Section {
         int start;
         int count;
      };

      int sections;
      int unknown;
      Section *section;
      U32 count;
      U32 countButLonger; // No clue what this is
      VertexData *data;
   };

   class GBitmap
   {
   public:
	   VTABLE(0);

	   VIRTFN(void, destroy, (bool del), (del), TGEVIRT_GBITMAP_DELETE);

	   ResourceObject *mSourceResource;

	   enum BitmapFormat {
		   Palettized = 0,
		   Intensity  = 1,
		   RGB        = 2,
		   RGBA       = 3,
		   Alpha      = 4,
		   RGB565     = 5,
		   RGB5551    = 6,
		   Luminance  = 7
	   };
	   BitmapFormat internalFormat;

	   U8* pBits;            // Master bytes
	   U32 byteSize;
	   U32 width;            // Top level w/h
	   U32 height;
	   U32 bytesPerPixel;

	   U32 numMipLevels;
	   U32 mipLevelOffsets[10];

	   void *pPalette;
   };

    class TextureObject
    {
    public:
        TextureObject *next;
        TextureObject *prev;
        TextureObject *hashNext;
        
        unsigned int        mGLTextureName;
        unsigned int        mGLTextureNameSmall;
        
        const char *        mTextureKey;
		U32                 unknown;
        U32                 mTextureWidth;
        U32                 mTextureHeight;
        U32                 mBitmapWidth;
        U32                 mBitmapHeight;
        U32                 mAnotherWidth;
        U32                 mAnotherHeight;
        unsigned int        mFilter;
        bool                mClamp;
    };

   class TextureHandle
   {
   public:
      TextureObject *object;

		/**
		 * Get the ACTUAL texture when referencing from InteriorExtension.cpp
		 */
	   TextureObject *getActualTexture() {
		   if (this->object) {
			   //Why does this work. What the fuck.
			   return this->object->prev;
		   }
		   return NULL;
	   }
   };

   class MaterialList
   {
   public:
      int *something;
      U32 dunno;
      TGE::Vector<char *> mTextureNames;
      TGE::Vector<TextureHandle *> mMaterials;
      int somethingElse;
      TGE::Vector<char *> mNotSures;
      char *defaultTexture;
   };

	namespace TextureManager
	{
		FN(TGE::TextureObject *, loadTexture, (const char *path, U32 type, bool clampToEdge), TGEADDR_TEXTUREMANAGER_LOADTEXTURE);
		FN(TGE::GBitmap *, loadBitmapInstance, (const char *path), TGEADDR_TEXTUREMANAGER_LOADBITMAPINSTANCE);
		FN(TGE::TextureObject *, registerTexture, (const char *textureName, const GBitmap *data, bool clampToEdge), TGEADDR_TEXTUREMANAGER_REGISTERTEXTURE);
		FN(bool, createGLName, (TGE::GBitmap *pBitmap, bool clampToEdge, U32 firstMip, U32 type, TGE::TextureObject *to), TGEADDR_TEXTUREMANAGER_CREATEGLNAME);
	};

	class GuiControlProfile : public SimObject
	{
	public:
		enum AlignmentType {
			LeftJustify,
			RightJustify,
			CenterJustify
		};

		MEMBERFNSIMP(U32, constructBitmapArray, TGEADDR_GUICONTROLPROFILE_CONSTRUCTBITMAPARRAY);
		GETTERFNSIMP(RectI *, getBitmapBounds, TGEOFF_GUICONTROLPROFILE_BITMAPBOUNDS)
		GETTERFNSIMP(TextureHandle, getTextureHandle, TGEOFF_GUICONTROLPROFILE_TEXTUREHANDLE);
		GETTERFNSIMP(AlignmentType, getJustify, TGEOFF_GUICONTROLPROFILE_JUSTIFY);
		GETTERFNSIMP(Point2I, getTextOffset, TGEOFF_GUICONTROLPROFILE_TEXTOFFSET);
		GETTERFNSIMP(bool, getBorder, TGEOFF_GUICONTROLPROFILE_BORDER);
	};


   class Interior {
   public:
	   struct ItrPaddedPoint
	   {
		   Point3F point;
		   union
		   {
				F32   fogCoord;
				U8    fogColor[4];
		   };
	   };
	   struct TexMatrix
	   {
		   S32 T;
		   S32 N;
		   S32 B;

		   TexMatrix()
		   :  T( -1 ),
		   N( -1 ),
		   B( -1 )
		   {};
	   };
	   struct ColorF {
		   F32 r;
		   F32 g;
		   F32 b;
		   F32 a;
	   };
	   struct IBSPNode
	   {
		   U16 planeIndex;
		   U32 frontIndex;
		   U32 backIndex;

		   U16 terminalZone;
	   };
	   struct IBSPLeafSolid
	   {
		   U32 surfaceIndex;
		   U16 surfaceCount;
	   };
	   struct TexGenPlanes
	   {
		   PlaneF planeX;
		   PlaneF planeY;
	   };
	   struct TriFan
	   {
		   U32 windingStart;
		   U32 windingCount;
	   };
	   struct Surface
	   {
		   U32 windingStart;          // 1
		   U16 planeIndex;            // 2
		   U16 textureIndex;
		   U32 texGenIndex;           // 3
		   U16 lightCount;            // 4
		   U8  surfaceFlags;
		   U8  windingCount;
		   char noIdea[6];
		   U8  mapOffsetX;            // 7
		   U8  mapOffsetY;
		   U8  mapSizeX;
		   U8  mapSizeY;
	   };

	   struct NullSurface
	   {
		   U32 windingStart;
		   U16 planeIndex;
		   U8  surfaceFlags;
		   U32 windingCount;
	   };
	   U32                     mFileVersion;
	   U32                     mDetailLevel;
	   U32                     mMinPixels;
	   F32                     mAveTexGenLength;     // Set in Interior::read after loading the texgen planes.
	   Box3F                   mBoundingBox;
	   SphereF                 mBoundingSphere;

	   F32 mSomething;

	   Vector<PlaneF>          mPlanes;
	   Vector<ItrPaddedPoint>  mPoints;
	   Vector<U8>              mPointVisibility;

	   ColorF                  mBaseAmbient;
	   ColorF                  mAlarmAmbient;

	   Vector<IBSPNode>        mBSPNodes;
	   Vector<IBSPLeafSolid>   mBSPSolidLeaves;

	   bool                    mPreppedForRender;
	   MaterialList*           mMaterialList;
	   TextureHandle*          mWhite;
	   TextureHandle*          mWhiteRGB;
	   
	   TextureHandle*          mLightFalloff;

	   Vector<TextureHandle*>  mEnvironMaps;
	   Vector<F32>             mEnvironFactors;
	   U32                     mValidEnvironMaps;

	   Vector<U32>             mWindings;
	   Vector<TexGenPlanes>    mTexGenEQs;
	   Vector<TexGenPlanes>    mLMTexGenEQs;

	   Vector<TriFan>          mWindingIndices;
	   Vector<Surface>         mSurfaces;
	   Vector<NullSurface>     mNullSurfaces;
	   Vector<U32>             mSolidLeafSurfaces;
	   

		MEMBERFN(void, renderSmooth, (MaterialList *list, ItrFastDetail *itr, bool a, int b, unsigned int c), (list, itr, a, b, c), TGEADDR_INTERIOR_RENDERSMOOTH);
		//MEMBERFN(void, renderLights, (void *info, MatrixF const &mat, Point3F const &pt, unsigned int *a, unsigned int b), (info, mat, pt, a, b), TGEADDR_INTERIOR_RENDERLIGHTS);
   };

	class InteriorResource {

	};

	class InteriorInstance : public SceneObject
	{
	public:
		MEMBERFNSIMP(bool, onAdd, TGEADDR_INTERIORINSTANCE_ONADD);
		MEMBERFN(Interior*, getDetailLevel, (U32 detailLevel), (detailLevel), TGEADDR_INTERIORINSTANCE_GETDETAILLEVEL);
		MEMBERFN(void, renderObject, (SceneState *state, SceneRenderImage *renderImage), (state, renderImage), TGEADDR_INTERIORINSTANCE_RENDEROBJECT);
		GETTERFNSIMP(const char *, getInteriorFile, TGEOFF_INTERIORINSTANCE_INTERIORFILE);
	};

	class TSStatic : public SceneObject
	{
	public:
		GETTERFNSIMP(const char *, getShapeName, TGEOFF_TSSTATIC_SHAPENAME);
	};

	class TSShape {
	public:
		struct ConvexHullAccelerator {
			S32      numVerts;
			Point3F* vertexList;
			Point3F* normalList;
			U8**     emitStrings;
		};
		MEMBERFN(void, computeAccelerator, (S32 dl), (dl), TGEADDR_TSSHAPE_COMPUTEACCELERATOR);
		GETTERFNSIMP(ResourceObject*, getSourceResource, TGEOFF_TSSHAPE_SOURCERESOURCE);
		GETTERFNSIMP(TGE::Vector<ConvexHullAccelerator*>, getDetailCollisionAccelerators, TGEOFF_TSSHAPE_DETAILCOLLISIONACCELERATORS);
		SETTERFN(TGE::Vector<ConvexHullAccelerator*>, setDetailCollisionAccelerators, TGEOFF_TSSHAPE_DETAILCOLLISIONACCELERATORS);
	};
	class Container
	{
	public:
		MEMBERFN(bool, castRay, (const Point3F &start, const Point3F &end, U32 mask, RayInfo *info), (start, end, mask, info), TGEADDR_CONTAINER_CASTRAY);
	};
	

	class ResourceObject
	{
	public:
		ResourceObject *prev, *next;

		ResourceObject *nextEntry;    ///< This is used by ResDictionary for its hash table.

		ResourceObject *nextResource;
		ResourceObject *prevResource;

		S32 flags;  ///< Set from Flags.

		const char *path;     ///< Resource path.
		const char *name;     ///< Resource name.

		/// @name ZIP Archive
		/// If the resource is stored in a zip file, these members are populated.
		/// @{

		///
		const char *zipPath;  ///< Path of zip file.
		const char *zipName;  ///< Name of zip file.

		S32 fileOffset;            ///< Offset of data in zip file.
		S32 fileSize;              ///< Size on disk of resource block.
		S32 compressedFileSize;    ///< Actual size of resource data.
		/// @}

		class ResourceInstance *mInstance;  ///< Pointer to actual object instance. If the object is not loaded,
		///  this may be NULL or garbage.
		S32 lockCount;                ///< Lock count; used to control load/unload of resource from memory.
		U32 crc;                      ///< CRC of resource.
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
		MEMBERFN(void, freeResource, (ResourceObject *res), (res), TGEADDR_RESMANAGER_FREERESOURCE);
		MEMBERFN(ResourceObject *, createResource, (const char *path, const char *file), (path, file), TGEADDR_RESMANAGER_CREATERESOURCE);
	};

	class GameConnection : public NetConnection
	{
	public:
		GETTERFNSIMP(SimObject *, getControlObject, TGEOFF_GAMECONNECTION_CONTROLOBJECT);
		MEMBERFN(bool, getControlCameraTransform, (F32 dt, MatrixF *outMat), (dt, outMat), TGEADDR_GAMECONNECTION_GETCONTROLCAMERATRANSFORM);
	};

	class PathedInterior : public GameBase
	{
	public:
		MEMBERFN(void, advance, (double delta), (delta), TGEADDR_PATHEDINTERIOR_ADVANCE);
		MEMBERFN(void, computeNextPathStep, (U32 delta), (delta), TGEADDR_PATHEDINTERIOR_COMPUTENEXTPATHSTEP);

		MEMBERFNSIMP(Point3F, getVelocity, TGEADDR_PATHEDINTERIOR_GETVELOCITY);

		GETTERFNSIMP(MatrixF, getBaseTransform, TGEOFF_PATHEDINTERIOR_BASETRANSFORM);
		SETTERFN(MatrixF, setBaseTransform, TGEOFF_PATHEDINTERIOR_BASETRANSFORM);

		GETTERFNSIMP(Point3F, getBaseScale, TGEOFF_PATHEDINTERIOR_BASESCALE);
		SETTERFN(Point3F, setBaseScale, TGEOFF_PATHEDINTERIOR_BASESCALE);

		GETTERFNSIMP(Point3F, getVelocity2, TGEOFF_PATHEDINTERIOR_VELOCITY);
		SETTERFN(Point3F, setVelocity, TGEOFF_PATHEDINTERIOR_VELOCITY);

		GETTERFNSIMP(F64, getPathPosition, TGEOFF_PATHEDINTERIOR_PATHPOSITION);
		SETTERFN(F64, setPathPosition, TGEOFF_PATHEDINTERIOR_PATHPOSITION);

		GETTERFNSIMP(F32, getPathPositionF32, TGEOFF_PATHEDINTERIOR_PATHPOSITIONFLOAT);
		SETTERFN(F32, setPathPositionF32, TGEOFF_PATHEDINTERIOR_PATHPOSITIONFLOAT);

		GETTERFNSIMP(S32, getTargetPosition, TGEOFF_PATHEDINTERIOR_TARGETPOSITION);
		SETTERFN(S32, setTargetPosition, TGEOFF_PATHEDINTERIOR_TARGETPOSITION);

		GETTERFNSIMP(Point3F, getOffset, TGEOFF_PATHEDINTERIOR_OFFSET);
		SETTERFN(Point3F, setOffset, TGEOFF_PATHEDINTERIOR_OFFSET);

		GETTERFNSIMP(U32, getPathKey, TGEOFF_PATHEDINTERIOR_PATHKEY);
		SETTERFN(U32, setPathKey, TGEOFF_PATHEDINTERIOR_PATHKEY);
		MEMBERFNSIMP(U32, getPathKey2, TGEADDR_PATHEDINTERIOR_GETPATHKEY);

		GETTERFNSIMP(const char *, getInteriorResource, TGEOFF_PATHEDINTERIOR_INTERIORRESOURCE);
		GETTERFNSIMP(U32, getInteriorIndex, TGEOFF_PATHEDINTERIOR_INTERIORINDEX);

		SETTERFN(const char *, setInteriorResource, TGEOFF_PATHEDINTERIOR_INTERIORRESOURCE);
		SETTERFN(U32, setInteriorIndex, TGEOFF_PATHEDINTERIOR_INTERIORINDEX);

		MEMBERFN(void, processTick, (const Move *move), (move), TGEADDR_PATHEDINTERIOR_PROCESSTICK);
		MEMBERFN(void, renderObject, (SceneState *state, SceneRenderImage *renderImage), (state, renderImage), TGEADDR_PATHEDINTERIOR_RENDEROBJECT);

		MEMBERFN(U32, parent_packUpdate, (TGE::NetConnection* con, U32 mask, TGE::BitStream* stream),(con,mask,stream), TGEADDR_PATHEDINTERIOR_GAMEBASE_PACKUPDATE);
		MEMBERFN(void, parent_unpackUpdate, (TGE::NetConnection* con, TGE::BitStream* stream), (con, stream), TGEADDR_PATHEDINTERIOR_GAMEBASE_UNPACKUPDATE);

		GETTERFNSIMP(PathedInterior *, getNext, TGEOFF_PATHEDINTERIOR_NEXTCLIENTPI);
	};

	namespace Namespace
	{
		FN(void, init, (), TGEADDR_NAMESPACE_INIT);

		class Namespace {
		public:
			GETTERFNSIMP(const char *, getName, TGEOFF_NAMESPACE_NAME);
			GETTERFNSIMP(Namespace *, getParent, TGEOFF_NAMESPACE_PARENT);
		};
	}

	class _StringTable
	{
	public:
		MEMBERFN(const char*, insert, (const char *string, bool caseSens), (string, caseSens), TGEADDR__STRINGTABLE_INSERT);
	};

	// Event types
	enum EventType
	{
		InputEventType = 0,
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
	class MarbleUpdateEvent
	{
	public:
		MEMBERFN(void, unpack, (TGE::NetConnection *connection, TGE::BitStream *stream), (connection, stream), TGEADDR_MARBLEUPDATEEVENT_UNPACK);
		MEMBERFN(void, pack, (TGE::NetConnection *connection, TGE::BitStream *stream), (connection, stream), TGEADDR_MARBLEUPDATEEVENT_PACK);
	};
	
#define SI_MAKE      0x01
#define SI_BREAK     0x02
#define SI_MOVE      0x03
#define SI_REPEAT    0x04

	///Device Event Types
#define SI_UNKNOWN   0x01
#define SI_BUTTON    0x02
#define SI_POV       0x03
#define SI_KEY       0x0A
#define SI_TEXT      0x10

	/// Event SubTypes
#define SI_ANY       0xff

	// Modifier Keys
	/// shift and ctrl are the same between platforms.
#define SI_LSHIFT    (1<<0)
#define SI_RSHIFT    (1<<1)
#define SI_SHIFT     (SI_LSHIFT|SI_RSHIFT)
#define SI_LCTRL     (1<<2)
#define SI_RCTRL     (1<<3)
#define SI_CTRL      (SI_LCTRL|SI_RCTRL)
	/// win altkey, mapped to mac cmdkey.
#define SI_LALT      (1<<4)
#define SI_RALT      (1<<5)
#define SI_ALT       (SI_LALT|SI_RALT)
	/// mac optionkey
#define SI_MAC_LOPT  (1<<6)
#define SI_MAC_ROPT  (1<<7)
#define SI_MAC_OPT   (SI_MAC_LOPT|SI_MAC_ROPT)

	enum KeyCodes {
		KEY_NULL = 0x000,     ///< Invalid KeyCode
		KEY_BACKSPACE = 0x001,
		KEY_TAB = 0x002,
		KEY_RETURN = 0x003,
		KEY_CONTROL = 0x004,
		KEY_ALT = 0x005,
		KEY_SHIFT = 0x006,
		KEY_PAUSE = 0x007,
		KEY_CAPSLOCK = 0x008,
		KEY_ESCAPE = 0x009,
		KEY_SPACE = 0x00a,
		KEY_PAGE_DOWN = 0x00b,
		KEY_PAGE_UP = 0x00c,
		KEY_END = 0x00d,
		KEY_HOME = 0x00e,
		KEY_LEFT = 0x00f,
		KEY_UP = 0x010,
		KEY_RIGHT = 0x011,
		KEY_DOWN = 0x012,
		KEY_PRINT = 0x013,
		KEY_INSERT = 0x014,
		KEY_DELETE = 0x015,
		KEY_HELP = 0x016,

		KEY_0 = 0x017,
		KEY_1 = 0x018,
		KEY_2 = 0x019,
		KEY_3 = 0x01a,
		KEY_4 = 0x01b,
		KEY_5 = 0x01c,
		KEY_6 = 0x01d,
		KEY_7 = 0x01e,
		KEY_8 = 0x01f,
		KEY_9 = 0x020,

		KEY_A = 0x021,
		KEY_B = 0x022,
		KEY_C = 0x023,
		KEY_D = 0x024,
		KEY_E = 0x025,
		KEY_F = 0x026,
		KEY_G = 0x027,
		KEY_H = 0x028,
		KEY_I = 0x029,
		KEY_J = 0x02a,
		KEY_K = 0x02b,
		KEY_L = 0x02c,
		KEY_M = 0x02d,
		KEY_N = 0x02e,
		KEY_O = 0x02f,
		KEY_P = 0x030,
		KEY_Q = 0x031,
		KEY_R = 0x032,
		KEY_S = 0x033,
		KEY_T = 0x034,
		KEY_U = 0x035,
		KEY_V = 0x036,
		KEY_W = 0x037,
		KEY_X = 0x038,
		KEY_Y = 0x039,
		KEY_Z = 0x03a,

		KEY_TILDE = 0x03b,
		KEY_MINUS = 0x03c,
		KEY_EQUALS = 0x03d,
		KEY_LBRACKET = 0x03e,
		KEY_RBRACKET = 0x03f,
		KEY_BACKSLASH = 0x040,
		KEY_SEMICOLON = 0x041,
		KEY_APOSTROPHE = 0x042,
		KEY_COMMA = 0x043,
		KEY_PERIOD = 0x044,
		KEY_SLASH = 0x045,
		KEY_NUMPAD0 = 0x046,
		KEY_NUMPAD1 = 0x047,
		KEY_NUMPAD2 = 0x048,
		KEY_NUMPAD3 = 0x049,
		KEY_NUMPAD4 = 0x04a,
		KEY_NUMPAD5 = 0x04b,
		KEY_NUMPAD6 = 0x04c,
		KEY_NUMPAD7 = 0x04d,
		KEY_NUMPAD8 = 0x04e,
		KEY_NUMPAD9 = 0x04f,
		KEY_MULTIPLY = 0x050,
		KEY_ADD = 0x051,
		KEY_SEPARATOR = 0x052,
		KEY_SUBTRACT = 0x053,
		KEY_DECIMAL = 0x054,
		KEY_DIVIDE = 0x055,
		KEY_NUMPADENTER = 0x056,

		KEY_F1 = 0x057,
		KEY_F2 = 0x058,
		KEY_F3 = 0x059,
		KEY_F4 = 0x05a,
		KEY_F5 = 0x05b,
		KEY_F6 = 0x05c,
		KEY_F7 = 0x05d,
		KEY_F8 = 0x05e,
		KEY_F9 = 0x05f,
		KEY_F10 = 0x060,
		KEY_F11 = 0x061,
		KEY_F12 = 0x062,
		KEY_F13 = 0x063,
		KEY_F14 = 0x064,
		KEY_F15 = 0x065,
		KEY_F16 = 0x066,
		KEY_F17 = 0x067,
		KEY_F18 = 0x068,
		KEY_F19 = 0x069,
		KEY_F20 = 0x06a,
		KEY_F21 = 0x06b,
		KEY_F22 = 0x06c,
		KEY_F23 = 0x06d,
		KEY_F24 = 0x06e,

		KEY_NUMLOCK = 0x06f,
		KEY_SCROLLLOCK = 0x070,
		KEY_LCONTROL = 0x071,
		KEY_RCONTROL = 0x072,
		KEY_LALT = 0x073,
		KEY_RALT = 0x074,
		KEY_LSHIFT = 0x075,
		KEY_RSHIFT = 0x076,
		KEY_WIN_LWINDOW = 0x077,
		KEY_WIN_RWINDOW = 0x078,
		KEY_WIN_APPS = 0x079,
		KEY_OEM_102 = 0x080,

		KEY_MAC_OPT = 0x090,
		KEY_MAC_LOPT = 0x091,
		KEY_MAC_ROPT = 0x092,

		KEY_BUTTON0 = 0x0100,
		KEY_BUTTON1 = 0x0101,
		KEY_BUTTON2 = 0x0102,
		KEY_BUTTON3 = 0x0103,
		KEY_BUTTON4 = 0x0104,
		KEY_BUTTON5 = 0x0105,
		KEY_BUTTON6 = 0x0106,
		KEY_BUTTON7 = 0x0107,
		KEY_BUTTON8 = 0x0108,
		KEY_BUTTON9 = 0x0109,
		KEY_BUTTON10 = 0x010A,
		KEY_BUTTON11 = 0x010B,
		KEY_BUTTON12 = 0x010C,
		KEY_BUTTON13 = 0x010D,
		KEY_BUTTON14 = 0x010E,
		KEY_BUTTON15 = 0x010F,
		KEY_BUTTON16 = 0x0110,
		KEY_BUTTON17 = 0x0111,
		KEY_BUTTON18 = 0x0112,
		KEY_BUTTON19 = 0x0113,
		KEY_BUTTON20 = 0x0114,
		KEY_BUTTON21 = 0x0115,
		KEY_BUTTON22 = 0x0116,
		KEY_BUTTON23 = 0x0117,
		KEY_BUTTON24 = 0x0118,
		KEY_BUTTON25 = 0x0119,
		KEY_BUTTON26 = 0x011A,
		KEY_BUTTON27 = 0x011B,
		KEY_BUTTON28 = 0x011C,
		KEY_BUTTON29 = 0x011D,
		KEY_BUTTON30 = 0x011E,
		KEY_BUTTON31 = 0x011F,
		KEY_ANYKEY = 0xfffe
	};

	/// Input device types
	enum InputDeviceTypes
	{
		UnknownDeviceType,
		MouseDeviceType,
		KeyboardDeviceType,
		JoystickDeviceType,
	};

	struct InputEvent : public Event
	{
		U32   deviceInst;
		float fValue;
		U16   deviceType;
		U16   objType;
		U16   ascii;
		U16   objInst;
		U8    action;
		U8    modifier;
		InputEvent() { type = InputEventType; size = sizeof(InputEvent); }
	};

	class GameInterface
	{
	public:
		VTABLE(TGEOFF_GAMEINTERFACE_VTABLE);

		VIRTFN(void, postEvent, (Event &ev), (ev), TGEVIRT_GAMEINTERFACE_POSTEVENT);
	};

	class DemoGame : public GameInterface
	{
	public:
		MEMBERFN(void, processTimeEvent, (TimeEvent *event), (event), TGEADDR_DEMOGAME_PROCESSTIMEEVENT);
		MEMBERFN(void, processInputEvent, (InputEvent *event), (event), TGEADDR_DEMOGAME_PROCESSINPUTEVENT);
		GETTERFNSIMP(bool, getDemoRecording, TGEOFF_DEMOGAME_DEMORECORDING);
		SETTERFN(bool, setDemoRecording, TGEOFF_DEMOGAME_DEMORECORDING);
		GETTERFNSIMP(bool, getDemoPlaying, TGEOFF_DEMOGAME_DEMOPLAYING);
		SETTERFN(bool, setDemoPlaying, TGEOFF_DEMOGAME_DEMOPLAYING);
		GETTERFNSIMP(BitStream *, getDemoStream, TGEOFF_DEMOGAME_DEMOSTREAM);
	};

	struct CameraQuery {
		CameraQuery() {
			ortho = false;
		}

		SimObject  *object;
		F32         nearPlane;
		F32         farPlane;
		F32         fov;
		MatrixF     cameraMatrix;

		F32         leftRight;
		F32         topBottom;
		bool        ortho;
	};

	class SceneGraph {
	public:
		MEMBERFN(void, renderScene, (unsigned int a), (a), TGEADDR_SCENEGRAPH_RENDERSCENE);
	};

	class ShowTSCtrl : public GuiControl {
	public:
		MEMBERFN(void, renderWorld, (const RectI &updateRect), (updateRect), TGEADDR_SHOWTSCTRL_RENDERWORLD);
		MEMBERFN(void, processCameraQuery, (CameraQuery *query), (query), TGEADDR_SHOWTSCTRL_PROCESSCAMERAQUERY);
	};

	class EditTSCtrl : public GuiControl {
	public:
		MEMBERFN(void, renderWorld, (const RectI &updateRect), (updateRect), TGEADDR_EDITTSCTRL_RENDERWORLD);
	};

	/// Represents a single GUI event.
	///
	/// This is passed around to all the relevant controls so they know what's going on.
	struct GuiEvent {
		U16                  ascii;            ///< ascii character code 'a', 'A', 'b', '*', etc (if device==keyboard) - possibly a uchar or something
		U8                   modifier;         ///< SI_LSHIFT, etc
		U32                  keyCode;          ///< for unprintables, 'tab', 'return', ...
		Point2I              mousePoint;       ///< for mouse events
		U8                   mouseClickCount;  ///< to determine double clicks, etc...
		U8                   mouseAxis;        ///< mousewheel axis (0 == X, 1 == Y)
		F32                  fval;             ///< used for mousewheel events

		GuiEvent()
		: ascii(0),
		modifier(0),
		keyCode(0),
		mousePoint(0, 0),
		mouseClickCount(0),
		mouseAxis(0),
		fval(0.f) {}
	};

	/// Represent a mouse event with a 3D position and vector.
	///
	/// This event used by the EditTSCtrl derived controls.
	struct Gui3DMouseEvent : public GuiEvent {
		Point3F     vec;
		Point3F     pos;

		Gui3DMouseEvent()
		: vec(0.f, 0.f, 0.f),
		pos(0.f, 0.f, 0.f) {}
	};

	class WorldEditor : public EditTSCtrl {
	public:
		MEMBERFN(void, on3DMouseDragged, (const Gui3DMouseEvent &event), (event), TGEADDR_WORLDEDITOR_ON3DMOUSEDRAGGED);
	};

	class PathManager {
	public:
		MEMBERFN(U32, getPathTotalTime, (U32 id), (id), TGEADDR_PATHMANAGER_GETPATHTOTALTIME);
		MEMBERFN(void, getPathPosition, (U32 id, F64 msPosition, Point3F &rPosition), (id, msPosition, rPosition), TGEADDR_PATHMANAGER_GETPATHPOSITION);
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

		//executef()
		OVERLOAD_PTR {
			OVERLOAD_FN(const char*, (SimObject *obj, int argc, ...), TGEADDR_CON_EXECUTEF_OBJECT);
			OVERLOAD_FN(const char*, (int argc, ...), TGEADDR_CON_EXECUTEF);
		} executef;

		//execute()
		OVERLOAD_PTR {
			OVERLOAD_FN(const char*, (S32 argc, const char *argv[]),                 TGEADDR_CON_EXECUTE);
			OVERLOAD_FN(const char*, (SimObject *obj, S32 argc, const char *argv[]), TGEADDR_CON_EXECUTE_OBJECT);
		} execute;

		// Variables
		FN(const char *, getVariable,      (const char *name),                    TGEADDR_CON_GETVARIABLE);
		FN(bool, getBoolVariable, (const char *name), TGEADDR_CON_GETBOOLVARIABLE);
		FN(S32, getIntVariable, (const char *name), TGEADDR_CON_GETINTVARIABLE);
		FN(F32, getFloatVariable, (const char *name), TGEADDR_CON_GETFLOATVARIABLE);
		FN(void,         setVariable,      (const char *name, const char *value), TGEADDR_CON_SETVARIABLE);
		FN(void,         setLocalVariable, (const char *name, const char *value), TGEADDR_CON_SETLOCALVARIABLE);
		FN(void,         setBoolVariable,  (const char *name, bool value),        TGEADDR_CON_SETBOOLVARIABLE);
		FN(void,         setIntVariable,   (const char *name, S32 value),         TGEADDR_CON_SETINTVARIABLE);
		FN(void,         setFloatVariable, (const char *name, F32 value),         TGEADDR_CON_SETFLOATVARIABLE);

		// Misc
		FN(const char*, evaluate,             (const char *string, bool echo, const char *fileName),       TGEADDR_CON_EVALUATE);
		FN(const char*, evaluatef,            (const char* string, ...),                                   TGEADDR_CON_EVALUATEF);
		FN(char*,       getReturnBuffer,      (U32 bufferSize),                                            TGEADDR_CON_GETRETURNBUFFER);
		FN(bool,        expandScriptFilename, (char *filename, U32 size, const char *src),                 TGEADDR_CON_EXPANDSCRIPTFILENAME);
		FN(const char*, getData,              (S32 type, void *dptr, S32 index, void *tbl, const BitSet32 *flag), TGEADDR_CON_GETDATA);
		FN(void,        setData,              (S32 type, void *dptr, S32 index, S32 argc, const char **argv, void *tbl, const BitSet32 *flag), TGEADDR_CON_SETDATA);
		FN(const char*, getTypeName,          (S32 type),                                                  TGEADDR_CON_GETTYPENAME);
		FN(bool,        isFunction,           (const char *name),                                          TGEADDR_CON_ISFUNCTION);
	}

	namespace Platform
	{
		FN(bool, dumpPath, (const char *path, Vector<FileInfo>& fileVector), TGEADDR_PLATFORM_DUMPPATH);
		FN(const char*, getWorkingDirectory, (), TGEADDR_PLATFORM_GETWORKINGDIRECTORY);
		FN(bool, isSubDirectory, (const char *parent, const char *child), TGEADDR_PLATFORM_ISSUBDIRECTORY);
		FN(bool, getFileTimes, (const char *path, FileTime *createTime, FileTime *modifyTime), TGEADDR_PLATFORM_GETFILETIMES);
		FN(U32, getRealMilliseconds, (), TGEADDR_PLATFORM_GETREALMILLISECONDS);
	}

	namespace ParticleEngine
	{
		// Initialization
		FN(void, init, (), TGEADDR_PARTICLEENGINE_INIT);
	}

	namespace Sim
	{
		FN(SimObject*, findObject, (const char *name), TGEADDR_SIM_FINDOBJECT);
		FN(SimObject*, findObject_int, (SimObjectId id), TGEADDR_SIM_FINDOBJECT_INT);
		FN(void, cancelEvent, (U32 eventSequence), TGEADDR_SIM_CANCELEVENT);
		GLOBALVAR(U32, gCurrentTime, TGEADDR_SIM_GCURRENTTIME);
		GLOBALVAR(U32, gTargetTime, TGEADDR_SIM_GCURRENTTIME-4);
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

	namespace Video
	{
		FN(void, reactivate, (bool force), TGEADDR_VIDEO_REACTIVATE);
		FN(void, deactivate, (bool force), TGEADDR_VIDEO_DEACTIVATE);
	}

	namespace Members
	{
		namespace OpenGLDevice
		{
			RAWMEMBERFN(TGE::OpenGLDevice, bool, activate, (U32 width, U32 height, U32 bpp, bool fullScreen), 0x4033B9);
			RAWMEMBERFN(TGE::OpenGLDevice, void, shutdown, (bool force), 0x40914C);
			RAWMEMBERFN(TGE::OpenGLDevice, bool, setScreenMode, (U32 width, U32 height, U32 bpp, bool fullScreen, bool forceIt, bool repaint), 0x406366);
		}

		namespace SimObject
		{
			RAWMEMBERFNSIMP(TGE::SimObject, void, deleteObject, TGEADDR_SIMOBJECT_DELETEOBJECT);
			RAWMEMBERFNSIMP(TGE::SimObject, bool, registerObject, TGEADDR_SIMOBJECT_REGISTEROBJECT);
			RAWMEMBERFN(TGE::SimObject, const char *, getDataField, (const char *slotName, const char *array), TGEADDR_SIMOBJECT_GETDATAFIELD);
			RAWMEMBERFN(TGE::SimObject, void, setDataField, (const char *slotName, const char *array, const char *value), TGEADDR_SIMOBJECT_SETDATAFIELD);
		}
		namespace NetObject
		{
			RAWMEMBERFN(TGE::NetObject, void, setMaskBits, (U32 bits), TGEADDR_NETOBJECT_SETMASKBITS);
			RAWMEMBERFNSIMP(TGE::NetObject, bool, onAdd, TGEADDR_NETOBJECT_ONADD);
			RAWMEMBERFN(TGE::NetObject, U32, packUpdate, (NetConnection *connection, U32 mask, TGE::BitStream *stream), TGEADDR_NETOBJECT_PACKUPDATE);
			RAWMEMBERFN(TGE::NetObject, void, unpackUpdate, (NetConnection *connection, TGE::BitStream *stream), TGEADDR_NETOBJECT_UNPACKUPDATE);
		}

		namespace TSShapeInstance
		{
			RAWMEMBERFN(TGE::TSShapeInstance, void, advanceTime, (F32 delta), 0x4045FC);
		}

		namespace TSThread
		{
			RAWMEMBERFN(TGE::TSThread, void, advancePos, (F32 delta), 0x408A53);
		}

		namespace ShapeBase
		{
			RAWMEMBERFN(TGE::ShapeBase, void, renderObject, (void *sceneState, void *sceneRenderImage), TGEADDR_SHAPEBASE_RENDEROBJECT);
			RAWMEMBERFN(TGE::ShapeBase, U32, packUpdate, (NetConnection *connection, U32 mask, TGE::BitStream *stream), TGEADDR_SHAPEBASE_PACKUPDATE);
			RAWMEMBERFN(TGE::ShapeBase, void, unpackUpdate, (NetConnection *connection, TGE::BitStream *stream), TGEADDR_SHAPEBASE_UNPACKUPDATE);
			RAWMEMBERFN(TGE::ShapeBase, void, setHidden, (bool hidden), TGEADDR_SHAPEBASE_SETHIDDEN);
			RAWMEMBERFN(TGE::ShapeBase, void, updateThread, (Thread& st), TGEADDR_SHAPEBASE_UPDATETHREAD);
			RAWMEMBERFNSIMP(TGE::ShapeBase, bool, onAdd, TGEADDR_SHAPEBASE_ONADD);
			RAWMEMBERFNSIMP(TGE::ShapeBase, void, onRemove, TGEADDR_SHAPEBASE_ONREMOVE);
		}

		namespace Trigger
		{
			RAWMEMBERFNSIMP(TGE::Trigger, bool, onAdd, TGEADDR_TRIGGER_ONADD);
			RAWMEMBERFNSIMP(TGE::Trigger, void, onEditorEnable, TGEADDR_TRIGGER_ONEDITORENABLE);
			RAWMEMBERFNSIMP(TGE::Trigger, void, onEditorDisable, TGEADDR_TRIGGER_ONEDITORDISABLE);
			RAWMEMBERFN(TGE::Trigger, void, renderObject, (void *sceneState, void *sceneRenderImage), TGEADDR_TRIGGER_RENDEROBJECT);
			RAWMEMBERFN(TGE::Trigger, U32, packUpdate, (NetConnection *connection, U32 mask, TGE::BitStream *stream), TGEADDR_TRIGGER_PACKUPDATE);
			RAWMEMBERFN(TGE::Trigger, void, unpackUpdate, (NetConnection *connection, TGE::BitStream *stream), TGEADDR_TRIGGER_UNPACKUPDATE);
		}


		namespace SimDataBlock
		{
			RAWMEMBERFN(TGE::SimDataBlock, void, packData, (TGE::BitStream *stream), TGEADDR_SIMDATABLOCK_PACKDATA);
		}

		namespace MarbleData
		{
			RAWMEMBERFN(TGE::MarbleData, void, packData, (TGE::BitStream *stream), TGEADDR_MARBLEDATA_PACKDATA);
		}

		namespace SceneObject
		{
			RAWMEMBERFN(TGE::SceneObject, void, setTransform, (const MatrixF &transform), TGEADDR_SCENEOBJECT_SETTRANSFORM);
			RAWMEMBERFN(TGE::SceneObject, void, setRenderTransform, (const MatrixF &transform), TGEADDR_SCENEOBJECT_SETRENDERTRANSFORM);
			RAWMEMBERFN(TGE::SceneObject, void, setScale, (const VectorF &scale), TGEADDR_SHAPEBASE_SETSCALE);
		}

		namespace Sun
		{
			RAWMEMBERFNSIMP(TGE::Sun, bool, onAdd, TGEADDR_SUN_ONADD);
		}

		namespace Sky
		{
			RAWMEMBERFNSIMP(TGE::Sky, bool, onAdd, TGEADDR_SKY_ONADD);
			RAWMEMBERFN(TGE::Sky, U32, packUpdate, (TGE::NetConnection *connection, U32 mask, TGE::BitStream *stream), TGEADDR_SKY_PACKUPDATE);
			RAWMEMBERFN(TGE::Sky, void, unpackUpdate, (TGE::NetConnection *connection, TGE::BitStream *stream), TGEADDR_SKY_UNPACKUPDATE);
		}

		namespace DemoGame
		{
			RAWMEMBERFN(TGE::DemoGame, void, processTimeEvent, (struct TimeEvent *event), TGEADDR_DEMOGAME_PROCESSTIMEEVENT);
			RAWMEMBERFN(TGE::DemoGame, void, processInputEvent, (InputEvent *event), TGEADDR_DEMOGAME_PROCESSINPUTEVENT);
			RAWMEMBERFNSIMP(TGE::DemoGame, void, textureKill, TGEADDR_DEMOGAME_TEXTUREKILL);
			RAWMEMBERFNSIMP(TGE::DemoGame, void, textureResurrect, TGEADDR_DEMOGAME_TEXTURERESURRECT);
		}

		namespace Marble
		{
			RAWMEMBERFN(TGE::Marble, void, doPowerUp, (S32 powerUpId), TGEADDR_MARBLE_DOPOWERUP);
			RAWMEMBERFN(TGE::Marble, void, advancePhysics, (const Move *move, U32 delta), TGEADDR_MARBLE_ADVANCEPHYSICS);
			RAWMEMBERFN(TGE::Marble, void, advanceCamera, (const Move *move, U32 delta), TGEADDR_MARBLE_ADVANCECAMERA);
			RAWMEMBERFN(TGE::Marble, U32, packUpdate, (TGE::NetConnection *connection, U32 mask, TGE::BitStream *stream), TGEADDR_MARBLE_PACKUPDATE);
			RAWMEMBERFN(TGE::Marble, void, unpackUpdate, (TGE::NetConnection *connection, TGE::BitStream *stream), TGEADDR_MARBLE_UNPACKUPDATE);
			RAWMEMBERFN(TGE::Marble, void, setPosition, (const Point3D &position, const AngAxisF &camera, F32 pitch), TGEADDR_MARBLE_SETPOSITION);
			RAWMEMBERFN(TGE::Marble, void, setPositionSimple, (const Point3D &position), TGEADDR_MARBLE_SETPOSITION_SIMPLE);
			RAWMEMBERFN(TGE::Marble, void, setTransform, (const MatrixF &mat), TGEADDR_MARBLE_SETTRANSFORM);
			RAWMEMBERFNSIMP(TGE::Marble, bool, onAdd, TGEADDR_MARBLE_ONADD);
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

		namespace TSShape
		{
			RAWMEMBERFN(TGE::TSShape, bool, read, (TGE::Stream *stream), TGEADDR_TSSHAPE_READ);
			RAWMEMBERFN(TGE::TSShape, void, computeAccelerator, (S32 dl), TGEADDR_TSSHAPE_COMPUTEACCELERATOR);
		}

		namespace InteriorInstance
		{
			RAWMEMBERFN(TGE::InteriorInstance, U32, packUpdate, (TGE::NetConnection *conn, U32 mask, TGE::BitStream *stream), TGEADDR_INTERIORINSTANCE_PACKUPDATE);
			RAWMEMBERFN(TGE::InteriorInstance, void, unpackUpdate, (TGE::NetConnection *conn, TGE::BitStream *stream), TGEADDR_INTERIORINSTANCE_UNPACKUPDATE);
			RAWMEMBERFNSIMP(TGE::InteriorInstance, bool, onAdd, TGEADDR_INTERIORINSTANCE_ONADD);
			RAWMEMBERFN(TGE::InteriorInstance, TGE::Interior*, getDetailLevel, (U32 detailLevel), TGEADDR_INTERIORINSTANCE_GETDETAILLEVEL);
			RAWMEMBERFN(TGE::InteriorInstance, void, renderObject, (SceneState *state, SceneRenderImage *renderImage), TGEADDR_INTERIORINSTANCE_RENDEROBJECT);
		}

		namespace FileStream
		{
			RAWMEMBERFN(TGE::FileStream, bool, open, (const char *path, int accessMode), TGEADDR_FILESTREAM_OPEN);
		}

		namespace PathedInterior
		{
			RAWMEMBERFN(TGE::PathedInterior, void, advance, (double delta), TGEADDR_PATHEDINTERIOR_ADVANCE);
			RAWMEMBERFN(TGE::PathedInterior, void, computeNextPathStep, (U32 delta), TGEADDR_PATHEDINTERIOR_COMPUTENEXTPATHSTEP);
			RAWMEMBERFN(TGE::PathedInterior, void, renderObject, (SceneState *state, SceneRenderImage *renderImage), TGEADDR_PATHEDINTERIOR_RENDEROBJECT);
			RAWMEMBERFN(TGE::PathedInterior, void, processTick, (const TGE::Move *move), TGEADDR_PATHEDINTERIOR_PROCESSTICK);
			RAWMEMBERFN(TGE::PathedInterior, U32, packUpdate, (TGE::NetConnection* con, U32 mask, TGE::BitStream* stream), TGEADDR_PATHEDINTERIOR_PACKUPDATE);
			RAWMEMBERFN(TGE::PathedInterior, void, unpackUpdate, (TGE::NetConnection* con,TGE::BitStream* stream), TGEADDR_PATHEDINTERIOR_UNPACKUPDATE);
			RAWMEMBERFNSIMP(TGE::PathedInterior, Point3F, getVelocity, TGEADDR_PATHEDINTERIOR_GETVELOCITY);
			RAWMEMBERFNSIMP(TGE::PathedInterior, bool, onAdd, TGEADDR_PATHEDINTERIOR_ONADD);
			RAWMEMBERFNSIMP(TGE::PathedInterior, void, onRemove, TGEADDR_PATHEDINTERIOR_ONREMOVE);
		}

		namespace ParticleEmitter
		{
			RAWMEMBERFNSIMP(TGE::ParticleEmitter, bool, onAdd, TGEADDR_PARTICLEEMITTER_ONADD);
			RAWMEMBERFNSIMP(TGE::ParticleEmitter, void, onRemove, TGEADDR_PARTICLEEMITTER_ONREMOVE);
			RAWMEMBERFN(TGE::ParticleEmitter, void, renderObject, (SceneState* state, SceneRenderImage* image), TGEADDR_PARTICLEEMITTER_RENDEROBJECT);
			RAWMEMBERFN(TGE::ParticleEmitter, void, emitParticles, (const Point3F& start, const Point3F& end, const Point3F& axis, const Point3F& velocity, const U32 numMilliseconds), TGEADDR_PARTICLEEMITTER_EMITPARTICLES);
			RAWMEMBERFN(TGE::ParticleEmitter, void, advanceTime, (F32 dt), 0x402B5D);
		}

		namespace PEEngine
		{
			RAWMEMBERFN(TGE::PEEngine, void, updateSingleParticle, (TGE::Particle* particle, TGE::ParticleEmitter& emitter, const U32 ms), 0x409494);
		}

		namespace AbstractClassRep
		{
			FN(void, initialize, (), TGEADDR_ABSTRACTCLASSREP_INITIALIZE);
		}

		// Only Mac has TCP object improvements
#ifdef __APPLE__
		namespace TCPObject
		{
			RAWMEMBERFNSIMP(TGE::TCPObject, void, onAdd, TGEADDR_TCPOBJECT_ONADD);
			RAWMEMBERFNSIMP(TGE::TCPObject, void, ctor, TGEADDR_TCPOBJECT_CTOR);
			RAWMEMBERFNSIMP(TGE::TCPObject, void, dtor, TGEADDR_TCPOBJECT_DTOR);
			RAWMEMBERFN(TGE::TCPObject, void, connect, (U32 argc, const char **argv), TGEADDR_TCPOBJECT_CONNECT);
			RAWMEMBERFN(TGE::TCPObject, void, send, (U32 argc, const char **argv), TGEADDR_TCPOBJECT_SEND);
		}
#endif // __APPLE__

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

		namespace GuiControl
		{
			RAWMEMBERFN(TGE::GuiControl, void, onRender, (Point2I offset, const RectI &rect), TGEADDR_GUICONTROL_ONRENDER);
		}

		namespace GuiCanvas
		{
			RAWMEMBERFN(TGE::GuiCanvas, void, renderFrame, (bool bufferSwap), TGEADDR_CANVAS_RENDERFRAME);
		}

		namespace GuiWindowCtrl
		{
			RAWMEMBERFN(TGE::GuiWindowCtrl, void, resize, (Point2I const&, Point2I const&), TGEADDR_GUIWINDOWCTRL_RESIZE)
		}

		namespace GuiBitmapCtrl
		{
			RAWMEMBERFN(TGE::GuiBitmapCtrl, void, onRender, (Point2I offset, const RectI &updateRect), TGEADDR_GUIBITMAPCTRL_ONRENDER);
		}

		namespace GuiBorderButtonCtrl
		{
			RAWMEMBERFN(TGE::GuiBorderButtonCtrl, void, onRender, (Point2I offest, const RectI &updateRect), TGEADDR_GUIBORDERBUTTONCTRL_ONRENDER);
		}

		namespace ShowTSCtrl {
			RAWMEMBERFN(TGE::ShowTSCtrl, void, renderWorld, (const RectI &updateRect), TGEADDR_SHOWTSCTRL_RENDERWORLD);
			RAWMEMBERFN(TGE::ShowTSCtrl, void, processCameraQuery, (CameraQuery *query), TGEADDR_SHOWTSCTRL_PROCESSCAMERAQUERY);
		}

		namespace EditTSCtrl {
			RAWMEMBERFN(TGE::EditTSCtrl, void, renderWorld, (const RectI &updateRect), TGEADDR_EDITTSCTRL_RENDERWORLD);
		}

		namespace WorldEditor {
			RAWMEMBERFN(TGE::WorldEditor, void, on3DMouseDragged, (const Gui3DMouseEvent &event), TGEADDR_WORLDEDITOR_ON3DMOUSEDRAGGED);
		}

		namespace GameConnection {
			RAWMEMBERFN(TGE::GameConnection, bool, getControlCameraTransform, (F32 dt, MatrixF *outMat), TGEADDR_GAMECONNECTION_GETCONTROLCAMERATRANSFORM);
		}

		namespace SceneGraph {
			RAWMEMBERFN(TGE::SceneGraph, void, renderScene, (unsigned int a), TGEADDR_SCENEGRAPH_RENDERSCENE);
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

		namespace Interior
		{
			RAWMEMBERFN(TGE::Interior, void, renderSmooth, (TGE::MaterialList *mat, ItrFastDetail *itr, bool a, int b, unsigned int c), TGEADDR_INTERIOR_RENDERSMOOTH);
			//RAWMEMBERFN(TGE::Interior, void, renderLights, (void *info, MatrixF const &mat, Point3F const &pt, unsigned int *a, unsigned int b), TGEADDR_INTERIOR_RENDERLIGHTS);
		}

		namespace InteriorResource
		{
			RAWMEMBERFN(TGE::InteriorResource, bool, read, (TGE::Stream &stream), TGEADDR_INTERIORRESOURCE_READ);
		}

		namespace Item
		{
			RAWMEMBERFN(TGE::Item, void, renderImage, (TGE::SceneState* state, void* image), 0x4010F0);
			RAWMEMBERFN(TGE::Item, void, advanceTime, (F32 dt), 0x0406AB9);
		}

		namespace MarbleUpdateEvent
		{
			RAWMEMBERFN(TGE::MarbleUpdateEvent, void, unpack, (TGE::NetConnection *connection, TGE::BitStream *stream), TGEADDR_MARBLEUPDATEEVENT_UNPACK);
			RAWMEMBERFN(TGE::MarbleUpdateEvent, void, pack, (TGE::NetConnection *connection, TGE::BitStream *stream), TGEADDR_MARBLEUPDATEEVENT_PACK);
		}

		namespace ShapeBase
		{
			RAWMEMBERFNSIMP(TGE::ShapeBase, bool, isHidden, TGEADDR_SHAPEBASE_ISHIDDEN);

		}

		namespace Shadow
		{
			RAWMEMBERFN(TGE::Shadow, void, setRadius, (void *shapeInstance, const Point3F &scale), TGEADDR_SHADOW_SETRADIUS);
		}

		namespace PathManager
		{
			RAWMEMBERFN(TGE::PathManager, U32, getPathTotalTime, (U32 id), TGEADDR_PATHMANAGER_GETPATHTOTALTIME);
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

		namespace GBitmap
		{
			RAWMEMBERFN(TGE::GBitmap, void, allocateBitmap, (U32 in_width, U32 in_height, bool in_extrudeMipLevels, TGE::GBitmap::BitmapFormat in_format), TGEADDR_GBITMAP_ALLOCATEBITMAP);
		}

		namespace SimFieldDictionary
		{
			RAWMEMBERFN(TGE::SimFieldDictionary, void, writeFields, (TGE::SimObject *obj, TGE::Stream &stream, U32 tabStop), TGEADDR_SIMFIELDDICTIONARY_WRITEFIELDS);
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
	FN(void, GameRenderWorld, (), TGEADDR_GAMERENDERWORLD);
	FN(void, dglSetClipRect, (const RectI &clipRect), TGEADDR_DGLSETCLIPRECT);
	FN(void, dglSetCanonicalState, (), TGEADDR_DGLSETCANONICALSTATE);
	FN(bool, GameGetCameraTransform, (MatrixF *mat, Point3F *pos), TGEADDR_GAMEGETCAMERATRANSFORM);

	FN(void, cSetTransform, (TGE::SimObject *obj, int argc, const char **argv), TGEADDR_CSETTRANSFORM);
	FN(void, cMarbleSetPosition, (TGE::Marble *obj, int argc, const char **argv), TGEADDR_CMARBLESETPOSITION);
	FN(void, cSetGravityDir, (TGE::SimObject *obj, int argc, const char **argv), TGEADDR_CSETGRAVITYDIR);
	FN(bool, cSimObjectSave, (TGE::SimObject *obj, int argc, const char **argv), TGEADDR_CSIMOBJECTSAVE);
	FN(S32, cGetRealTime, (TGE::SimObject *obj, int argc, const char **argv), TGEADDR_CGETREALTIME);
	FN(S32, cGetSimTime, (TGE::SimObject *obj, int argc, const char **argv), TGEADDR_CGETSIMTIME);
	FN(S32, cGetObject, (TGE::SimObject *obj, int argc, const char **argv), 0x43CF10);

	FN(void, m_matF_x_box3F_C, (F32 *m, F32* min, F32* max), TGEADDR_M_MATF_X_BOX3F_C);
	FN(int, alxPlay, (int source), TGEADDR_ALXPLAY);
	FN(int, alxCreateSource, (AudioProfile* sfx, const MatrixF* transform), TGEADDR_ALXCREATESOURCE);
	FN(const char*, ceval, (SimObject *object, int argc, const char **argv), TGEADDR_CEVAL);

	FN(void, dglClearBitmapModulation, (), TGEADDR_DGLCLEARBITMAPMODULATION);

	FN(void, dglDrawBitmapSR, (TextureObject *texture, Point2I const &destRect, RectI const &srcRect, bool in_flip), TGEADDR_DGLDRAWBITMAPSR);
	FN(void, dglDrawBitmapStretchSR, (TextureObject *texture, RectI const &destRect, RectI const &srcRect, bool in_flip), TGEADDR_DGLDRAWBITMAPSTRETCHSR);

	FN(void*, op_new, (U32 size, const char *file, U32 line), TGEADDR_OPNEW);
	FN(void*, op_new_array, (U32 size, const char *file, U32 line), TGEADDR_OPNEWARRAY);

	// Global variables
	GLOBALVAR(Container, gClientContainer, TGEADDR_GCLIENTCONTAINER);
	GLOBALVAR(Container, gServerContainer, TGEADDR_GSERVERCONTAINER);
	GLOBALVAR(_StringTable*, StringTable, TGEADDR_STRINGTABLE);
	GLOBALVAR(ResManager*, ResourceManager, TGEADDR_RESOURCEMANAGER);
	GLOBALVAR(GameInterface*, Game, TGEADDR_GAME);
	GLOBALVAR(SceneGraph *, gClientSceneGraph, TGEADDR_GCLIENTSCENEGRAPH);
	GLOBALVAR(Point3F *, gGlobalGravityDir, TGEADDR_GGLOBALGRAVITYDIR);
	GLOBALVAR(MatrixF*, gGlobalGravityMatrix, 0x6A9E80);

	//PathedInterior::mClientPathedInteriors (start of a linked list)
	GLOBALVAR(PathedInterior *, gClientPathedInteriors, TGEADDR_PATHEDINTERIOR_MCLIENTPATHEDINTERIORS);

#ifdef __APPLE__
	GLOBALVAR(bool, gShowBoundingBox, TGEADDR_SHOWBOUNDINGBOX);
#endif // __APPLE__
	GLOBALVAR(bool, gGamePaused, TGEADDR_GAMEPAUSED);
	GLOBALVAR(GuiCanvas *, Canvas, TGEADDR_CANVAS);

	GLOBALVAR(PathManager *, gClientPathManager, TGEADDR_GCLIENTPATHMANAGER);
	GLOBALVAR(PathManager *, gServerPathManager, TGEADDR_GSERVERPATHMANAGER);

	GLOBALVAR(bool, sgTextureTrilinear, TGEADDR_SGTEXTURETRILINEAR);
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

// O hackery of hackeries
#define conmethod_return_const              return (const
#define conmethod_return_S32                return (S32
#define conmethod_return_F32                return (F32
#define conmethod_nullify(val)
#define conmethod_return_void               conmethod_nullify(void
#define conmethod_return_bool               return (bool

// Defines a console function.
#define ConsoleFunction(name, returnType, minArgs, maxArgs, usage)                         \
	static returnType c##name(TGE::SimObject *, S32, const char **argv);                   \
	static TGE::_ConsoleConstructor g##name##obj(#name, c##name, usage, minArgs, maxArgs); \
	static returnType c##name(TGE::SimObject *, S32 argc, const char **argv)

#define ConsoleMethod(className, name, type, minArgs, maxArgs, usage) \
	static type c##className##name(TGE::className *, S32, const char **argv); \
	static type c##className##name##caster(TGE::SimObject *object, S32 argc, const char **argv) { \
	if (!reinterpret_cast<TGE::className*>(object)) \
		TGE::Con::warnf("Object passed to " #name " is not a " #className "!"); \
		conmethod_return_##type ) c##className##name(static_cast<TGE::className*>(object),argc,argv); \
	}; \
	static TGE::_ConsoleConstructor g##className##name##obj(#className, #name, c##className##name##caster, usage, minArgs, maxArgs); \
	static type c##className##name(TGE::className *object, S32 argc, const char **argv)

#endif // IN_PLUGIN_LOADER

#endif // TORQUELIB_TGE_H
#endif // MAC DEFINED