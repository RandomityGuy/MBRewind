#include <TorqueLib/TGE.h>
#include "RewindApi.h"
#include <cstdio>
#include "StringMath.h"

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
	sprintf(buf, "%s::getState", BindingNamespace.c_str());
	return atoi(TGE::Con::executef(1, buf));;
}

template<>
float RewindableBinding<float>::getState()
{
	char buf[512];
	sprintf(buf, "%s::getState", BindingNamespace.c_str());
	return atof(TGE::Con::executef(1, buf));;
}

template<>
bool RewindableBinding<bool>::getState()
{
	char buf[512];
	sprintf(buf, "%s::getState", BindingNamespace.c_str());
	return atoi(TGE::Con::executef(1, buf));;
}

template<>
std::string RewindableBinding<std::string>::getState()
{
	char buf[512];
	sprintf(buf, "%s::getState", BindingNamespace.c_str());
	return std::string(TGE::Con::executef(1, buf));
}

// Get State (Object)
template<>
int RewindableBinding<int>::getState(TGE::SimObject* obj)
{
	char buf[512];
	sprintf(buf, "%s::getState", BindingNamespace.c_str());
	return atoi(TGE::Con::executef(2, buf, StringMath::print(obj->getId())));;
}

template<>
float RewindableBinding<float>::getState(TGE::SimObject* obj)
{
	char buf[512];
	sprintf(buf, "%s::getState", BindingNamespace.c_str());
	return atof(TGE::Con::executef(2, buf, StringMath::print(obj->getId())));;
}

template<>
bool RewindableBinding<bool>::getState(TGE::SimObject* obj)
{
	char buf[512];
	sprintf(buf, "%s::getState", BindingNamespace.c_str());
	return atoi(TGE::Con::executef(2, buf, StringMath::print(obj->getId())));;
}

template<>
std::string RewindableBinding<std::string>::getState(TGE::SimObject* obj)
{
	char buf[512];
	sprintf(buf, "%s::getState", BindingNamespace.c_str());
	return std::string(TGE::Con::executef(2, buf, StringMath::print(obj->getId())));;
}

// Set State (Variable)
template<typename T>
void RewindableBinding<T>::setState(T state)
{
	char buf[512];
	sprintf(buf, "%s::setState", BindingNamespace.c_str());
	TGE::Con::executef(2, buf, StringMath::print(state));
}


template<>
void RewindableBinding<std::string>::setState(std::string state)
{
	char buf[512];
	sprintf(buf, "%s::setState", BindingNamespace.c_str());
	TGE::Con::executef(2, buf, state.c_str());
}

// Set State (Object)
template<typename T>
void RewindableBinding<T>::setState(T state,TGE::SimObject* obj)
{
	char buf[512];
	sprintf(buf, "%s::setState", BindingNamespace.c_str());
	TGE::Con::executef(3, buf, StringMath::print(obj->getId()), StringMath::print(state));
}

template<>
void RewindableBinding<std::string>::setState(std::string state, TGE::SimObject* obj)
{
	char buf[512];
	sprintf(buf, "%s::setState", BindingNamespace.c_str());
	TGE::Con::executef(3, buf, StringMath::print(obj->getId()), state.c_str());
}

// Interpolate State (Variable)
template<>
int RewindableBinding<int>::interpolateState(int one, int two, float ratio, float delta)
{
	char buf[512];
	sprintf(buf, "%s::interpolateState", BindingNamespace.c_str());
	return atoi(TGE::Con::executef(5, buf, StringMath::print(one), StringMath::print(two), StringMath::print(ratio), StringMath::print(delta)));
}

template<>
float RewindableBinding<float>::interpolateState(float one, float two, float ratio, float delta)
{
	char buf[512];
	sprintf(buf, "%s::interpolateState", BindingNamespace.c_str());
	return atof(TGE::Con::executef(5, buf, StringMath::print(one), StringMath::print(two), StringMath::print(ratio), StringMath::print(delta)));
}

template<>
bool RewindableBinding<bool>::interpolateState(bool one, bool two, float ratio, float delta)
{
	char buf[512];
	sprintf(buf, "%s::interpolateState", BindingNamespace.c_str());
	return atoi(TGE::Con::executef(5, buf, StringMath::print(one), StringMath::print(two), StringMath::print(ratio), StringMath::print(delta)));
}

template<>
std::string RewindableBinding<std::string>::interpolateState(std::string one, std::string two, float ratio, float delta)
{
	char buf[512];
	sprintf(buf, "%s::interpolateState", BindingNamespace.c_str());
	return std::string(TGE::Con::executef(5, buf, one.c_str(), two.c_str(), StringMath::print(ratio), StringMath::print(delta)));
}

// OnRewind (Variable)
void RewindableBindingBase::onRewind()
{
	char buf[512];
	sprintf(buf,"%s::onRewind", BindingNamespace.c_str());
	TGE::Con::executef(1, buf);
}

// OnRewind (Object)
// OnRewind (Variable)
void RewindableBindingBase::onRewind(TGE::SimObject* obj)
{
	char buf[512];
	sprintf(buf, "%s::onRewind", BindingNamespace.c_str());
	TGE::Con::executef(2, buf, StringMath::print(obj->getId()));
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
template class RewindableBinding<int>;
template class RewindableBinding<float>;
template class RewindableBinding<bool>;
template class RewindableState<std::string>;