#include NAVBOT_PCH_FILE
#include "hl1mp_mod.h"
#include <bot/hl1mp/hl1mp_bot.h>

CHL1MPMod::CHL1MPMod()
{
}

CHL1MPMod::~CHL1MPMod()
{
}

CHL1MPMod* CHL1MPMod::GetHL1MPMod()
{
	return static_cast<CHL1MPMod*>(extmanager->GetMod());
}

CBaseBot* CHL1MPMod::AllocateBot(edict_t* edict)
{
	return new CHL1MPBot(edict);
}

class CHL1MPModHelpers : public IModHelpers
{
public:
	bool IsPlayableTeam(int teamNum) const override
	{
		return true;
	}
};

IModHelpers* CHL1MPMod::AllocModHelpers() const
{
	return new CHL1MPModHelpers;
}
