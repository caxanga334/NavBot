#ifndef SMNAV_ENTITIES_BASE_ENTITY_H_
#define SMNAV_ENTITIES_BASE_ENTITY_H_
#pragma once

// Helper classes to access entity data

struct edict_t;
class CBaseEntity;

namespace entities
{
	// 'H' prefix for Helper Base Entity, to avoid conflicts

	class HBaseEntity
	{
	public:
		HBaseEntity(edict_t* entity);
		HBaseEntity(CBaseEntity* entity);

		// returns an edict index or reference for non networked entities
		inline int GetIndex() const { return m_index; }
		bool GetEntity(CBaseEntity** entity, edict_t** edict) const;
		Vector GetAbsOrigin() const;
		QAngle GetAbsAngles() const;
		Vector GetAbsVelocity() const;

	private:
		int m_index; // entity index or reference
	};
}

#endif // !SMNAV_ENTITIES_BASE_ENTITY_H_

