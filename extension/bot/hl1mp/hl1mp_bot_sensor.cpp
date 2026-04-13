#include NAVBOT_PCH_FILE
#include "hl1mp_bot_sensor.h"
#include "hl1mp_bot.h"

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

void CHL1MPBotSensor::CollectPlayers(std::vector<edict_t*>& visibleVec)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CHL1MPBotSensor::CollectPlayers", "NavBot");
#endif // EXT_VPROF_ENABLED

	const int myindex = GetBot<CBaseBot>()->GetIndex();

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		// skip self
		if (i == myindex)
		{
			continue;
		}

		CBaseEntity* entity = gamehelpers->ReferenceToEntity(i);

		// removed team checks since players are on team 0 in hl1mp
		if (!entity || modhelpers->IsDead(entity))
		{
			continue;
		}

		if (!ISensor::IsInPVS(UtilHelpers::getEntityOrigin(entity)))
		{
			continue;
		}

		if (IsAbleToSee(entity))
		{
			visibleVec.push_back(UtilHelpers::BaseEntityToEdict(entity));
		}
	}
}
