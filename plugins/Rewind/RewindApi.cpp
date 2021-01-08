#include <TorqueLib/TGE.h>
#include "RewindApi.h"
#include <cstdio>

template <typename T>
RewindableBinding<T>::RewindableBinding(RewindableType bindingtype,std::string bindingnamespace)
{
	this->BindingNamespace = bindingnamespace;
	this->BindingType = bindingtype;
}

template<>
RewindableBinding<int>::RewindableBinding(RewindableType bindingtype, std::string bindingnamespace)
{
	this->BindingNamespace = bindingnamespace;
	this->BindingType = bindingtype;
}

template<>
RewindableBinding<float>::RewindableBinding(RewindableType bindingtype, std::string bindingnamespace)
{
	this->BindingNamespace = bindingnamespace;
	this->BindingType = bindingtype;
}

template<>
RewindableBinding<bool>::RewindableBinding(RewindableType bindingtype, std::string bindingnamespace)
{
	this->BindingNamespace = bindingnamespace;
	this->BindingType = bindingtype;
}

template<>
RewindableBinding<std::string>::RewindableBinding(RewindableType bindingtype, std::string bindingnamespace)
{
	this->BindingNamespace = bindingnamespace;
	this->BindingType = bindingtype;
}


// Get Storage Type
int RewindableBindingBase::getStorageType()
{
	return -1; //This should never be reached
}

template<>
int RewindableBinding<int>::getStorageType()
{
	return 0;
}
template<>
int RewindableBinding<float>::getStorageType()
{
	return 1;
}
template<>
int RewindableBinding<bool>::getStorageType()
{
	return 2;
}
template<>
int RewindableBinding<std::string>::getStorageType()
{
	return 3;
}

// Get State (Variable)
template<>
int RewindableBinding<int>::getState()
{
	char buf[512];
	sprintf(buf, "$tempAPIVar = %s::getState();", BindingNamespace.c_str());
	TGE::Con::evaluatef(buf);
	return TGE::Con::getIntVariable("$tempAPIVar");
}

template<>
float RewindableBinding<float>::getState()
{
	char buf[512];
	sprintf(buf, "$tempAPIVar = %s::getState();", BindingNamespace.c_str());
	TGE::Con::evaluatef(buf);
	return TGE::Con::getFloatVariable("$tempAPIVar");
}

template<>
bool RewindableBinding<bool>::getState()
{
	char buf[512];
	sprintf(buf, "$tempAPIVar = %s::getState();", BindingNamespace.c_str());
	TGE::Con::evaluatef(buf);
	return TGE::Con::getBoolVariable("$tempAPIVar");
}

template<>
std::string RewindableBinding<std::string>::getState()
{
	char buf[512];
	sprintf(buf, "$tempAPIVar = %s::getState();", BindingNamespace.c_str());
	TGE::Con::evaluatef(buf);
	return std::string(TGE::Con::getVariable("$tempAPIVar"));
}

// Get State (Object)
template<>
int RewindableBinding<int>::getState(TGE::SimObject* obj)
{
	char buf[512];
	sprintf(buf, "$tempAPIVar = %s::getState(%d);", BindingNamespace.c_str(),obj->getId());
	TGE::Con::evaluatef(buf);
	return TGE::Con::getIntVariable("$tempAPIVar");
}

template<>
float RewindableBinding<float>::getState(TGE::SimObject* obj)
{
	char buf[512];
	sprintf(buf, "$tempAPIVar = %s::getState(%d);", BindingNamespace.c_str(), obj->getId());
	TGE::Con::evaluatef(buf);
	return TGE::Con::getFloatVariable("$tempAPIVar");
}

template<>
bool RewindableBinding<bool>::getState(TGE::SimObject* obj)
{
	char buf[512];
	sprintf(buf, "$tempAPIVar = %s::getState(%d);", BindingNamespace.c_str(), obj->getId());
	TGE::Con::evaluatef(buf);
	return TGE::Con::getBoolVariable("$tempAPIVar");
}

template<>
std::string RewindableBinding<std::string>::getState(TGE::SimObject* obj)
{
	char buf[512];
	sprintf(buf, "$tempAPIVar = %s::getState(%d);", BindingNamespace.c_str(), obj->getId());
	TGE::Con::evaluatef(buf);
	return std::string(TGE::Con::getVariable("$tempAPIVar"));
}

// Set State (Variable)
template<>
void RewindableBinding<int>::setState(int state)
{
	char buf[512];
	sprintf(buf, "%s::setState(%d);", BindingNamespace.c_str(), state);
	TGE::Con::evaluatef(buf);
}

template<>
void RewindableBinding<float>::setState(float state)
{
	char buf[512];
	sprintf(buf, "%s::setState(%f);", BindingNamespace.c_str(), state);
	TGE::Con::evaluatef(buf);
}

template<>
void RewindableBinding<bool>::setState(bool state)
{
	char buf[512];
	sprintf(buf, "%s::setState(%d);", BindingNamespace.c_str(), state);
	TGE::Con::evaluatef(buf);
}

template<>
void RewindableBinding<std::string>::setState(std::string state)
{
	char buf[512];
	sprintf(buf, "%s::setState(%s);", BindingNamespace.c_str(), state.c_str());
	TGE::Con::evaluatef(buf);
}

// Set State (Object)
template<>
void RewindableBinding<int>::setState(int state,TGE::SimObject* obj)
{
	char buf[512];
	sprintf(buf, "%s::setState(%d,%d);", BindingNamespace.c_str(),obj->getId(), state);
	TGE::Con::evaluatef(buf);
}

template<>
void RewindableBinding<float>::setState(float state, TGE::SimObject* obj)
{
	char buf[512];
	sprintf(buf, "%s::setState(%d,%f);", BindingNamespace.c_str(),obj->getId(), state);
	TGE::Con::evaluatef(buf);
}

template<>
void RewindableBinding<bool>::setState(bool state, TGE::SimObject* obj)
{
	char buf[512];
	sprintf(buf, "%s::setState(%d,%d);", BindingNamespace.c_str(),obj->getId(), state);
	TGE::Con::evaluatef(buf);
}

template<>
void RewindableBinding<std::string>::setState(std::string state, TGE::SimObject* obj)
{
	char buf[512];
	sprintf(buf, "%s::setState(%d,%s);", BindingNamespace.c_str(),obj->getId(), state.c_str());
	TGE::Con::evaluatef(buf);
}

// Interpolate State (Variable)
template<>
int RewindableBinding<int>::interpolateState(int one, int two, float ratio, float delta)
{
	char buf[512];
	sprintf(buf,"$tempAPIVar = %s::interpolateState(%d,%d,%f,%f);", BindingNamespace.c_str(), one, two, ratio, delta);
	TGE::Con::evaluatef(buf);
	return TGE::Con::getIntVariable("$tempAPIVar");
}

template<>
float RewindableBinding<float>::interpolateState(float one, float two, float ratio, float delta)
{
	char buf[512];
	sprintf(buf, "$tempAPIVar = %s::interpolateState(%f,%f,%f,%f);", BindingNamespace.c_str(), one, two, ratio, delta);
	TGE::Con::evaluatef(buf);
	return TGE::Con::getFloatVariable("$tempAPIVar");
}

template<>
bool RewindableBinding<bool>::interpolateState(bool one, bool two, float ratio, float delta)
{
	char buf[512];
	sprintf(buf, "$tempAPIVar = %s::interpolateState(%d,%d,%f,%f);", BindingNamespace.c_str(), one, two, ratio, delta);
	TGE::Con::evaluatef(buf);
	return TGE::Con::getBoolVariable("$tempAPIVar");
}

template<>
std::string RewindableBinding<std::string>::interpolateState(std::string one, std::string two, float ratio, float delta)
{
	char buf[512];
	sprintf(buf, "$tempAPIVar = %s::interpolateState(%s,%s,%f,%f);", BindingNamespace.c_str(), one.c_str(), two.c_str(), ratio, delta);
	TGE::Con::evaluatef(buf);
	return std::string(TGE::Con::getVariable("$tempAPIVar"));
}

// OnRewind (Variable)
void RewindableBindingBase::onRewind()
{
	char buf[512];
	sprintf(buf,"%s::onRewind();", BindingNamespace.c_str());
	TGE::Con::evaluatef(buf);
}

// OnRewind (Object)
// OnRewind (Variable)
void RewindableBindingBase::onRewind(TGE::SimObject* obj)
{
	char buf[512];
	sprintf(buf,"%s::onRewind(%d);", BindingNamespace.c_str(),obj->getId());
	TGE::Con::evaluatef(buf);
}

// RewindableState IO
template <typename T>
RewindableState<T> RewindableState<T>::read(MemoryStream* f)
{
	RewindableState<T> state(f->readString());

	state.value = f->read<T>();

	return state;
}

template <typename T>
void RewindableState<T>::write(MemoryStream* f)
{
	f->writeString(this->bindingnamespace);
	f->write<T>(this->value);
}

template <typename T>
RewindableState<T>::RewindableState(std::string bindingnamespace)
{
	this->bindingnamespace = bindingnamespace;
}

template class RewindableState<int>;
template class RewindableState<float>;
template class RewindableState<bool>;
template class RewindableState<std::string>;