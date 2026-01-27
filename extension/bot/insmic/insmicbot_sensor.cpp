#include NAVBOT_PCH_FILE
#include <mods/insmic/insmicmod.h>
#include "insmicbot.h"
#include "insmicbot_sensor.h"

CInsMICBotSensor::CInsMICBotSensor(CInsMICBot* bot) :
	CSimplePvPSensor<CInsMICBot>(bot)
{
}

void CInsMICBotSensor::CollectPlayers(std::vector<edict_t*>& visibleVec)
{
	for (int i = 1; i < gpGlobals->maxClients; i++)
	{
		if (i == GetBot<CBaseBot>()->GetIndex())
		{
			continue;
		}

		SourceMod::IGamePlayer* player = playerhelpers->GetGamePlayer(i);

		if (!player || !player->IsConnected() || player->GetPlayerInfo() == nullptr)
		{
			continue;
		}

		IPlayerInfo* info = player->GetPlayerInfo();

		if (info->IsDead())
		{
			continue;
		}

		insmic::InsMICTeam team = static_cast<insmic::InsMICTeam>(info->GetTeamIndex());

		if (team == insmic::InsMICTeam::INSMICTEAM_UNASSINGED || team == insmic::InsMICTeam::INSMICTEAM_SPECTATOR)
		{
			continue;
		}

		edict_t* edict = player->GetEdict();

		if (!edict || !UtilHelpers::IsValidEdict(edict))
		{
			continue;
		}

		if (!ISensor::IsInPVS(info->GetAbsOrigin()))
		{
			continue;
		}

		if (IsAbleToSee(edict))
		{
			visibleVec.push_back(edict);
		}
	}
}
