#ifndef NAVBOT_UTIL_EHANDLE_EDICT_H_
#define NAVBOT_UTIL_EHANDLE_EDICT_H_

#include <basehandle.h>

struct edict_t;
class CBaseEntity;

/**
 * @brief A version of CHandle that returns edict_t instead of CBaseEntity
 */
class CHandleEdict : public CBaseHandle
{
public:

	CHandleEdict();
	CHandleEdict(int iEntry, int iSerialNumber);
	CHandleEdict(const CBaseHandle& handle);
	CHandleEdict(edict_t* entity);

	edict_t* Get() const;
	void Set(const edict_t* val);

	operator edict_t* () { return Get(); }
	operator edict_t* () const { return Get(); }

	bool	operator !() const { return Get() != nullptr; }
	bool	operator==(edict_t* val) const;
	bool	operator!=(edict_t* val) const;
	const CBaseHandle& operator=(const edict_t* val)
	{
		Set(val);
		return *this;
	}

	edict_t* operator->() const { return Get(); }
	// Tries to get a CBaseEntity pointer
	CBaseEntity* ToBaseEntity() const;
};

#endif // !NAVBOT_UTIL_EHANDLE_EDICT_H_
