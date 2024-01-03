#ifndef NAVBOT_ENTITIES_BCC_H_
#define NAVBOT_ENTITIES_BCC_H_
#pragma once

#include <entities/baseentity.h>

namespace entities
{
	class HBaseCombatCharacter : public HBaseEntity
	{
	public:
		HBaseCombatCharacter(edict_t* edict);
		HBaseCombatCharacter(CBaseEntity* entity);

		edict_t* GetActiveWeapon() const;
		bool HasWeapon(const char* classname) const;
	};
}

#endif // !NAVBOT_ENTITIES_BCC_H_
