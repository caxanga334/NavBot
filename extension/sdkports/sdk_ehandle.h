#ifndef NAVBOT_SDKPORTS_EHANDLE_H_
#define NAVBOT_SDKPORTS_EHANDLE_H_
#pragma once

#include <basehandle.h>

// -------------------------------------------------------------------------------------------------- //
// CHandle.
// -------------------------------------------------------------------------------------------------- //
template< class T >
class CHandle : public CBaseHandle
{
public:

	CHandle();
	CHandle(int iEntry, int iSerialNumber);
	CHandle(const CBaseHandle& handle);
	CHandle(T* pVal);

	// The index should have come from a call to ToInt(). If it hasn't, you're in trouble.
	static CHandle<T> FromIndex(int index);

	T* Get() const;
	void	Set(const T* pVal);

	operator T* ();
	operator T* () const;

	bool	operator !() const;
	bool	operator==(T* val) const;
	bool	operator!=(T* val) const;
	const CBaseHandle& operator=(const T* val);

	T* operator->() const;

	// Tries to get an edict pointer
	edict_t* ToEdict() const;
};


// ----------------------------------------------------------------------- //
// Inlines.
// ----------------------------------------------------------------------- //

template<class T>
CHandle<T>::CHandle()
{
}


template<class T>
CHandle<T>::CHandle(int iEntry, int iSerialNumber)
{
	Init(iEntry, iSerialNumber);
}


template<class T>
CHandle<T>::CHandle(const CBaseHandle& handle)
	: CBaseHandle(handle)
{
}


template<class T>
CHandle<T>::CHandle(T* pObj)
{
	Term();
	Set(pObj);
}


template<class T>
inline CHandle<T> CHandle<T>::FromIndex(int index)
{
	CHandle<T> ret;
	ret.m_Index = index;
	return ret;
}


template<class T>
inline T* CHandle<T>::Get() const
{
	return (T*)CBaseHandle::Get();
}


template<class T>
inline CHandle<T>::operator T* ()
{
	return Get();
}

template<class T>
inline CHandle<T>::operator T* () const
{
	return Get();
}


template<class T>
inline bool CHandle<T>::operator !() const
{
	return !Get();
}

template<class T>
inline bool CHandle<T>::operator==(T* val) const
{
	return Get() == val;
}

template<class T>
inline bool CHandle<T>::operator!=(T* val) const
{
	return Get() != val;
}

template<class T>
void CHandle<T>::Set(const T* pVal)
{
	CBaseHandle::Set(reinterpret_cast<const IHandleEntity*>(pVal));
}

template<class T>
inline const CBaseHandle& CHandle<T>::operator=(const T* val)
{
	Set(val);
	return *this;
}

template<class T>
T* CHandle<T>::operator -> () const
{
	return Get();
}

template<class T>
inline edict_t* CHandle<T>::ToEdict() const
{
	IServerUnknown* unk = reinterpret_cast<IServerUnknown*>(Get());

	if (!unk)
	{
		return nullptr;
	}

	return unk->GetNetworkable()->GetEdict();
}

typedef CHandle<CBaseEntity> EHANDLE;

#endif // !NAVBOT_SDKPORTS_EHANDLE_H_