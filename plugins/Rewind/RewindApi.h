#pragma once
#include <string>
#include <TorqueLib/TGE.h>
#include "MemoryStream.h"

/*
Rewind API Docs:

	registerBinding(namespace,type,storagetype)
	unregisterBinding(namespace)


Callbacks:

	Variable Binding:
		namespace::getState()
		namespace::setState(%state)
		namespace::interpolateState(%one,%two,%ratio,%delta)
		namespace::onRewind()
	Object Binding:
		namespace::getState(%obj)
		namespace::setState(%obj,%state)
		namespace::interpolateState(%one,%two,%ratio,%delta)
		namespace::onRewind(%obj)


BindingType
	Variable: normal getState, setState
	SceneObject: goes in MissionState

*/

const char* executefnmspc(const char* ns, const char* fn, S32 argc, ...);

enum RewindableType
{
	SceneObject = 0,
	Variable = 1
};

class RewindableBindingBase
{
public:
	RewindableType BindingType;
	std::string BindingNamespace;
	virtual int getStorageType();
	void onRewind();
	void onRewind(TGE::SimObject* obj);
};

template <typename T>
class RewindableBinding : public RewindableBindingBase
{
public:
	RewindableBinding(RewindableType bindingType, std::string bindingnamespace);

	int getStorageType();

	T getState();
	T getState(TGE::SimObject* obj);
	void setState(T state);
	void setState(T state,TGE::SimObject* obj);
	T interpolateState(T one, T two, float ratio, float delta);
};

template <typename T>
struct RewindableState
{
	T value;
	std::string bindingnamespace;
public:
	RewindableState(std::string bindingnamespace);
	static RewindableState read(MemoryStream* f);
	void write(MemoryStream* f);


};
