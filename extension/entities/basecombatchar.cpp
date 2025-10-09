#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/entprops.h>
#include "basecombatchar.h"

edict_t* entities::HBaseCombatCharacter::GetActiveWeapon() const
{
	int me = GetIndex();
	int weapon;

	if (entprops->GetEntPropEnt(me, Prop_Send, "m_hActiveWeapon", &weapon))
	{
		return gamehelpers->EdictOfIndex(weapon);
	}

	return nullptr;
}

bool entities::HBaseCombatCharacter::HasWeapon(const char* classname) const
{
	int me = GetIndex();

	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		int weapon;
		
		if (!entprops->GetEntPropEnt(me, Prop_Send, "m_hMyWeapons", &weapon, nullptr, i))
			continue;

		edict_t* entity = gamehelpers->EdictOfIndex(weapon);

		if (!entity)
			continue;

		auto name = gamehelpers->GetEntityClassname(entity);

		if (!name)
			continue;

		if (strcasecmp(name, classname) == 0)
		{
			return true;
		}
	}

	return false;
}


