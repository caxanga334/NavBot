#include <extension.h>
#include "helpers.h"
#include "ehandle_edict.h"

CHandleEdict::CHandleEdict() :
	CBaseHandle()
{
}

CHandleEdict::CHandleEdict(int iEntry, int iSerialNumber) :
	CBaseHandle(iEntry, iSerialNumber)
{
}

CHandleEdict::CHandleEdict(const CBaseHandle& handle) :
	CBaseHandle(handle)
{
}

CHandleEdict::CHandleEdict(edict_t* entity)
{
	Term();
	Set(entity);
}

edict_t* CHandleEdict::Get() const
{
	return UtilHelpers::GetEdictFromCBaseHandle(*this);
}

void CHandleEdict::Set(const edict_t* val)
{
	if (val == nullptr)
	{
		Term();
		return;
	}

	const IServerEntity* se = val->GetIServerEntity();

	if (se != nullptr)
	{
		CBaseHandle::Set(se);
		return;
	}

	Term();
}

bool CHandleEdict::operator==(edict_t* val) const
{
	int theirindex, myindex;

	if (Get() == nullptr)
	{
		return false;
	}

	myindex = gamehelpers->IndexOfEdict(Get());
	theirindex = gamehelpers->IndexOfEdict(val);

	return myindex == theirindex;
}

bool CHandleEdict::operator!=(edict_t* val) const
{
	int theirindex, myindex;

	if (Get() == nullptr)
	{
		return false;
	}

	myindex = gamehelpers->IndexOfEdict(Get());
	theirindex = gamehelpers->IndexOfEdict(val);

	return myindex != theirindex;
}

CBaseEntity* CHandleEdict::ToBaseEntity() const
{
	return UtilHelpers::GetBaseEntityFromCBaseHandle(*this);
}

