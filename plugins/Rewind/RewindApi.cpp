#include <TorqueLib/TGE.h>
#include "RewindApi.h"
#include <cstdio>
#include "StringMath.h"
#include <string>

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

int torqueAtoi(const char* str)
{
	if (strcmp(str, "true") == 0)
		return 1;
	if (strcmp(str, "false") == 0)
		return 0;
	if (strcmp(str, "") == 0)
		return 0;
	return atoi(str);
}

double torqueAtof(const char* str)
{
	if (strcmp(str, "true") == 0)
		return 1;
	if (strcmp(str, "false") == 0)
		return 0;
	if (strcmp(str, "") == 0)
		return 0;
	return atof(str);
}

TGE::Namespace::NamespaceEntry* lookup(TGE::Namespace::Namespace* nmspc, const char* ns, const char* fn)
{
	TGE::Namespace::Namespace* nm = nmspc;
	while (nm != NULL)
	{
		if (nm->mEntryList != NULL)
		{
			if (nm->mName != NULL)
			{
				if (strcmp(nm->mName, ns) == 0)
				{
					TGE::Namespace::NamespaceEntry* entry = nm->mEntryList;
					while (entry != NULL)
					{
						if (strcmp(entry->mFunctionName, fn) == 0)
							return entry;
						entry = entry->mNext;
					}
				}
			}
		}
		nm = nm->mNext;
	}
}

const char* executeNamespacedFn(const char* ns, const char* fn, int argc, const char* argv[])
{
	TGE::Namespace::Namespace* nmspc = TGE::Namespace::find(ns, 0);
	TGE::Namespace::NamespaceEntry* en = lookup(nmspc, ns, fn);// nmspc->lookup(TGE::StringTable->insert(fn, false));
	return en->execute(argc, argv, TGE::Namespace::gEvalState);
}

const char* executefnmspc(const char* ns, const char* fn, S32 argc, ...)
{
	const char* argv[128];

	va_list args;
	va_start(args, argc);
	argv[0] = fn;
	for (S32 i = 0; i < argc; i++)
		argv[i + 1] = va_arg(args, const char*);
	va_end(args);

	return executeNamespacedFn(ns, fn, argc + 1, argv);
}

// Get State (Variable)
template<>
int RewindableBinding<int>::getState()
{
	return torqueAtoi(executefnmspc(BindingNamespace.c_str(), "getState", 0));
}

template<>
float RewindableBinding<float>::getState()
{
	return torqueAtof(executefnmspc(BindingNamespace.c_str(), "getState", 0));
}

template<>
bool RewindableBinding<bool>::getState()
{
	return torqueAtoi(executefnmspc(BindingNamespace.c_str(), "getState", 0));
}

template<>
std::string RewindableBinding<std::string>::getState()
{
	return std::string(executefnmspc(BindingNamespace.c_str(), "getState", 0));
}

// Get State (Object)
template<>
int RewindableBinding<int>::getState(TGE::SimObject* obj)
{
	return torqueAtoi(executefnmspc(BindingNamespace.c_str(), "getState", 1, TGE::StringTable->insert(StringMath::print(obj->getId()), false)));
}

template<>
float RewindableBinding<float>::getState(TGE::SimObject* obj)
{
	return torqueAtof(executefnmspc(BindingNamespace.c_str(), "getState", 1, TGE::StringTable->insert(StringMath::print(obj->getId()), false)));
}

template<>
bool RewindableBinding<bool>::getState(TGE::SimObject* obj)
{
	return torqueAtoi(executefnmspc(BindingNamespace.c_str(), "getState", 1, TGE::StringTable->insert(StringMath::print(obj->getId()), false)));
}

template<>
std::string RewindableBinding<std::string>::getState(TGE::SimObject* obj)
{
	return std::string(executefnmspc(BindingNamespace.c_str(), "getState", 1, TGE::StringTable->insert(StringMath::print(obj->getId()), false)));
}

// Set State (Variable)
template<typename T>
void RewindableBinding<T>::setState(T state)
{
	executefnmspc(BindingNamespace.c_str(), "setState", 1, TGE::StringTable->insert(StringMath::print(state), false));
}


template<>
void RewindableBinding<std::string>::setState(std::string state)
{
	executefnmspc(BindingNamespace.c_str(), "setState", 1, state.c_str());
}

// Set State (Object)
template<typename T>
void RewindableBinding<T>::setState(T state,TGE::SimObject* obj)
{
	const char* stateval = TGE::StringTable->insert(StringMath::print(state), false);
	executefnmspc(BindingNamespace.c_str(), "setState", 2, obj->getIdString(), stateval);
}

template<>
void RewindableBinding<std::string>::setState(std::string state, TGE::SimObject* obj)
{
	executefnmspc(BindingNamespace.c_str(), "setState", 2, obj->getIdString(), state.c_str());
}

// Interpolate State (Variable)
template<>
int RewindableBinding<int>::interpolateState(int one, int two, float ratio, float delta)
{
	std::string oneptr = std::to_string(one);
	std::string twoptr = std::to_string(two);
	std::string ratioptr = std::to_string(ratio);
	std::string deltaptr = std::to_string(delta);
	return torqueAtoi(executefnmspc(BindingNamespace.c_str(), "interpolateState", 4, oneptr.c_str(), twoptr.c_str(), ratioptr.c_str(), deltaptr.c_str()));
}

template<>
float RewindableBinding<float>::interpolateState(float one, float two, float ratio, float delta)
{
	std::string oneptr = std::to_string(one);
	std::string twoptr = std::to_string(two);
	std::string ratioptr = std::to_string(ratio);
	std::string deltaptr = std::to_string(delta);
	return torqueAtof(executefnmspc(BindingNamespace.c_str(), "interpolateState", 4, oneptr.c_str(), twoptr.c_str(), ratioptr.c_str(), deltaptr.c_str()));
}

template<>
bool RewindableBinding<bool>::interpolateState(bool one, bool two, float ratio, float delta)
{
	std::string oneptr = std::to_string(one);
	std::string twoptr = std::to_string(two);
	std::string ratioptr = std::to_string(ratio);
	std::string deltaptr = std::to_string(delta);
	return torqueAtoi(executefnmspc(BindingNamespace.c_str(), "interpolateState", 4, oneptr.c_str(), twoptr.c_str(), ratioptr.c_str(), deltaptr.c_str()));
}

template<>
std::string RewindableBinding<std::string>::interpolateState(std::string one, std::string two, float ratio, float delta)
{
	const char* oneptr = one.c_str();
	const char* twoptr = two.c_str();
	std::string ratioptr = std::to_string(ratio);
	std::string deltaptr = std::to_string(delta);
	return std::string(executefnmspc(BindingNamespace.c_str(), "interpolateState", 4, oneptr, twoptr, ratioptr.c_str(), deltaptr.c_str()));
}

// OnRewind (Variable)
void RewindableBindingBase::onRewind()
{
	executefnmspc(BindingNamespace.c_str(), "onRewind", 0);
}

// OnRewind (Object)
// OnRewind (Variable)
void RewindableBindingBase::onRewind(TGE::SimObject* obj)
{
	executefnmspc(BindingNamespace.c_str(), "onRewind", 1, TGE::StringTable->insert(StringMath::print(obj->getId()), false));
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