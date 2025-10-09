#ifndef __NAVBOT_ENTITIES_BASEANIMATING_H_
#define __NAVBOT_ENTITIES_BASEANIMATING_H_

#include <entities/baseentity.h>

namespace entities
{
	class HBaseAnimating : public HBaseEntity
	{
	public:
		HBaseAnimating(edict_t* entity) : HBaseEntity(entity) {}
		HBaseAnimating(CBaseEntity* entity) : HBaseEntity(entity) {}


	
	};
}

#endif // !__NAVBOT_ENTITIES_BASEANIMATING_H_
