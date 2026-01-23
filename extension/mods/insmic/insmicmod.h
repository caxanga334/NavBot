#ifndef __NAVBOT_INSMIC_MOD_H_
#define __NAVBOT_INSMIC_MOD_H_

#include <mods/basemod.h>
#include "insmic_lib.h"

class CInsMICMod : public CBaseMod
{
public:

	CBaseBot* AllocateBot(edict_t* edict) override;
	IModHelpers* AllocModHelpers() const override;

private:

};

#endif // !__NAVBOT_INSMIC_MOD_H_
