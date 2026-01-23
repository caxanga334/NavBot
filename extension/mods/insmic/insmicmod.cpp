#include NAVBOT_PCH_FILE
#include <bot/insmic/insmicbot.h>
#include "insmic_modhelpers.h"
#include "insmicmod.h"

CBaseBot* CInsMICMod::AllocateBot(edict_t* edict)
{
    return new CInsMICBot(edict);
}

IModHelpers* CInsMICMod::AllocModHelpers() const
{
    return new CInsMICModHelpers;
}
