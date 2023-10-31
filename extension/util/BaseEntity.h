/*
 * CbaseClass.h
 *
 *  Created on: Apr 6, 2017
 */

#ifndef UTILS_VALVE_NAVMESH_UTIL_BASEENTITY_H_
#define UTILS_VALVE_NAVMESH_UTIL_BASEENTITY_H_

class CBaseEntity;
class SendTable;
struct edict_t;

class BaseEntity {
public:
	BaseEntity(edict_t* ent): ent(ent) {
	}

	virtual ~BaseEntity() {
	}

	int *getTeam() const {
		return getPtr<int>("m_iTeamNum");
	}

	int *getMoveType() const {
		return getPtr<int>("movetype");
	}

	unsigned char *getRenderMode() const {
		return getPtr<unsigned char>("m_nRenderMode");
	}

	int *getModelIndex() const {
		return getPtr<int>("m_nModelIndex");
	}

	int *getHealth() const {
		return getPtr<int>("m_iHealth");
	}

	bool isUnderWater() const {
		return get<int>("m_nWaterLevel", 0) > 1;
	}

	edict_t* getGroundEntity() const {
		return getEntity("m_hGroundEntity");
	}

	bool isDestroyedOrUsed() const;


	template<typename T>
	T get(const char* varName, const T defaultVal) const {
		T *out = getPtr<T>(varName);
		return out == nullptr ? defaultVal : *out;
	}

	template<typename T>
	T *getPtr(const char* varName) const {
		return reinterpret_cast<T*>(getPointer(varName));
	}

	int *getFlags() {
		return getPtr<int>("m_fFlags");
	}

	edict_t* getEntity(const char *varName) const;

protected:
	edict_t* ent = nullptr;

private:
	static int getOffset(const char* varName, SendTable* pTable, int offset);

	char *getPointer(const char* varName) const;
};

#endif /* UTILS_VALVE_NAVMESH_UTIL_BASEENTITY_H_ */
