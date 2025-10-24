#ifndef __NAVBOT_ENTITIES_PROPS_H_
#define __NAVBOT_ENTITIES_PROPS_H_

#include "baseanimating.h"

namespace entities
{
	class HBaseProp : public HBaseAnimating
	{
	public:
		HBaseProp(edict_t* entity) : HBaseAnimating(entity) {}
		HBaseProp(CBaseEntity* entity) : HBaseAnimating(entity) {}


	};

	class HBreakableProp : public HBaseProp
	{
	public:
		HBreakableProp(edict_t* entity) : HBaseProp(entity) {}
		HBreakableProp(CBaseEntity* entity) : HBaseProp(entity) {}


	};

	class HDynamicProp : public HBreakableProp
	{
	public:
		HDynamicProp(edict_t* entity) : HBreakableProp(entity) {}
		HDynamicProp(CBaseEntity* entity) : HBreakableProp(entity) {}


	};

	class HPhysicsProp : public HBreakableProp
	{
	public:
		HPhysicsProp(edict_t* entity) : HBreakableProp(entity) {}
		HPhysicsProp(CBaseEntity* entity) : HBreakableProp(entity) {}


	};
}

#endif // !__NAVBOT_ENTITIES_PROPS_H_
