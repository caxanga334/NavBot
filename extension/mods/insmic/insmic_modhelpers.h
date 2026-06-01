#ifndef __NAVBOT_INSMIC_MODHELPERS_H_
#define __NAVBOT_INSMIC_MODHELPERS_H_

#include <mods/modhelpers.h>

class CInsMICModHelpers : public IModHelpers
{
public:

	int GetEntityTeamNumber(CBaseEntity* entity) const override;
	bool IsCombatCharacter(CBaseEntity* entity) const override;
};

#endif // !__NAVBOT_INSMIC_MODHELPERS_H_
