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

bool zpslib::IsUseableTriggerDisabled(CBaseEntity* entity)
{
	static int offset = 0;

	if (offset == 0)
	{
		// needs to be a CTriggerUseable to get the offset
		if (!UtilHelpers::datamap::IsEntityOfClass(entity, "CTriggerUseable"))
		{
			return false;
		}

		// trigger_useable (CTriggerUseable) contains two properties named m_bDisabled.
		// we need to read the one that is a member of CTriggerUseable

		datamap_t* map = gamehelpers->GetDataMap(entity);
		int offs = UtilHelpers::datamap::FindOffsetNoInheritance(map, "m_bDisabled");

		if (offs <= 0)
		{
			return false;
		}

		offset = offs;
	}

	bool* disabled = entprops->GetPointerToEntData<bool>(entity, static_cast<unsigned int>(offset));
	return *disabled;
}

zps::ItemDeliverStates zpslib::GetItemDeliverState(CBaseEntity* entity)
{
	int state = 0;
	entprops->GetEntProp(entity, Prop_Send, "m_iItemState", state);
	return static_cast<zps::ItemDeliverStates>(state);
}

std::string zpslib::GetItemDeliverIDName(CBaseEntity* entity)
{
	return entprops->GetEntPropString(entity, Prop_Data, "m_strItemID");
}

float zpslib::GetPlayerWeight(CBaseEntity* player)
{
	float value = 0.0f;
	entprops->GetEntPropFloat(player, Prop_Send, "m_flPlayerWeight", value);
	return value;
}

bool zpslib::PlayerHasNamedItem(CBaseEntity* player, const char* id)
{
	CHandle<CBaseEntity>* myweapons = entprops->GetCachedDataPtr<CHandle<CBaseEntity>>(player, CEntPropUtils::CacheIndex::CBASECOMBATCHARACTER_MYWEAPONS);
	std::size_t size = entprops->GetEntPropArraySize(player, Prop_Send, "m_hMyWeapons");

	for (std::size_t i = 0; i < size; i++)
	{
		CBaseEntity* entity = myweapons[i].Get();

		if (!entity) { continue; }

		if (!UtilHelpers::FClassnameIs(entity, "item_deliver")) { continue; }

		std::string otherid = GetItemDeliverIDName(entity);

		if (ke::StrCaseCmp(otherid.c_str(), id) == 0)
		{
			return true;
		}
	}

	return false;
}
