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

	bool IsUseObstructed(CBaseEntity* player, CBaseEntity* entity, CBaseEntity** obstruction) const override
	{
		// see CBasePlayer::FindUseEntity()
		// https://github.com/ValveSoftware/source-sdk-2013/blob/88fa198fba3fb85d46d4c95018254693fdc3af0a/src/game/shared/baseplayer_shared.cpp#L1067

		constexpr int useableContents = MASK_SOLID;

		trace::CTraceFilterSimple filter(player, COLLISION_GROUP_NONE);
		trace_t tr;
		Vector eyePos;
		gameclients->ClientEarPosition(UtilHelpers::BaseEntityToEdict(player), &eyePos);
		Vector end = UtilHelpers::getWorldSpaceCenter(entity);

		trace::line(eyePos, end, useableContents, &filter, tr);

		if (tr.startsolid || tr.fraction < 1.0f)
		{
			if (tr.m_pEnt == entity)
			{
				return false;
			}

			if (obstruction)
			{
				*obstruction = tr.m_pEnt;
			}

			return true;
		}

		return false;
	}
};

IModHelpers* CHL1MPMod::AllocModHelpers() const
{
	return new CHL1MPModHelpers;
}
