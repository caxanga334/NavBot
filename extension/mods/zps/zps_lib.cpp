#include NAVBOT_PCH_FILE
#include <extension.h>
#include "../modhelpers.h"
#include "zps_lib.h"

zps::ZPSTeam zpslib::GetEntityZPSTeam(CBaseEntity* entity)
{
	int team = 0;
	entprops->GetEntProp(entity, Prop_Data, "m_iTeamNum", team);
	return static_cast<zps::ZPSTeam>(team);
}

bool zpslib::IsPlayerInfected(CBaseEntity* player)
{
	bool result = false;
	entprops->GetEntPropBool(player, Prop_Send, "m_bIsInfected", result);
	return result;
}

bool zpslib::IsPlayerWalking(CBaseEntity* player)
{
	bool result = false;
	entprops->GetEntPropBool(player, Prop_Send, "m_bIsWalking", result);
	return result;
}

bool zpslib::IsPlayerCarrier(CBaseEntity* player)
{
	bool result = false;
	entprops->GetEntPropBool(player, Prop_Send, "m_bIsCarrier", result);
	return result;
}

CBaseEntity* zpslib::GetCarrierZombie()
{
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity* entity = gamehelpers->ReferenceToEntity(i);

		if (entity && zpslib::IsPlayerCarrier(entity))
		{
			return entity;
		}
	}

	return nullptr;
}

CBaseEntity* zpslib::GetRandomLivingSurvivor(const bool nobots)
{
	std::vector<CBaseEntity*> candidates;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		SourceMod::IGamePlayer* player = playerhelpers->GetGamePlayer(i);

		if (!player) { continue; }
		if (!player->IsInGame()) { continue; }
		if (nobots && player->IsFakeClient()) { continue; }

		CBaseEntity* entity = gamehelpers->ReferenceToEntity(i);

		if (entity &&
			zpslib::GetEntityZPSTeam(entity) == zps::ZPSTeam::ZPS_TEAM_SURVIVORS &&
			modhelpers->IsAlive(entity))
		{
			candidates.push_back(entity);
		}
	}

	if (candidates.empty())
	{
		return nullptr;
	}

	if (candidates.size() == 1)
	{
		return candidates[0];
	}

	return librandom::utils::GetRandomElementFromVector(candidates);
}
