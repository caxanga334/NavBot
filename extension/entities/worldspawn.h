#ifndef SMNAV_ENTITIES_WORLD_SPAWN_H_
#define SMNAV_ENTITIES_WORLD_SPAWN_H_

#include "baseentity.h"

namespace entities
{
	class HWorld : public HBaseEntity
	{
	public:
		HWorld(edict_t* entity) : HBaseEntity(entity) {}
		HWorld(CBaseEntity* entity) : HBaseEntity(entity) {}

		Vector GetWorldMins() const;
		Vector GetWorldMaxs() const;
	};
}

#endif // !SMNAV_ENTITIES_WORLD_SPAWN_H_
