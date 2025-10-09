#ifndef NAVBOT_ENTITIES_BCC_H_
#define NAVBOT_ENTITIES_BCC_H_
#pragma once

#include <entities/baseanimating.h>

namespace entities
{
	class HBaseCombatCharacter : public HBaseAnimating
	{
	public:
		HBaseCombatCharacter(edict_t* edict) : HBaseAnimating(edict) {}
		HBaseCombatCharacter(CBaseEntity* entity) : HBaseAnimating(entity) {}

		edict_t* GetActiveWeapon() const;
		bool HasWeapon(const char* classname) const;
	};
}

#endif // !NAVBOT_ENTITIES_BCC_H_
