#ifndef __NAVBOT_MODS_HL1MP_MOD_H_
#define __NAVBOT_MODS_HL1MP_MOD_H_

#include "../basemod.h"

class CHL1MPMod : public CBaseMod
{
public:
	CHL1MPMod();
	~CHL1MPMod() override;
	static CHL1MPMod* GetHL1MPMod();
	CBaseBot* AllocateBot(edict_t* edict) override;
	IModHelpers* AllocModHelpers() const override;
	bool IsTeamBasedGame() const override { return false; } /* supposedly it does HL1DMS does have teamplay but it appears to be broken */

private:

};


#endif // !__NAVBOT_MODS_HL1MP_MOD_H_
