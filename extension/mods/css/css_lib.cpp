#include NAVBOT_PCH_FILE
#include "css_lib.h"

counterstrikesource::CSSTeam csslib::GetEntityCSSTeam(CBaseEntity* entity)
{
	int team = 0;
	entprops->GetEntProp(entity, Prop_Data, "m_iTeamNum", team);
	return static_cast<counterstrikesource::CSSTeam>(team);
}

int csslib::GetPlayerMoney(CBaseEntity* player)
{
	int money = 0;
	entprops->GetEntProp(player, Prop_Send, "m_iAccount", money);
	return money;
}

int csslib::GetPlayerClass(CBaseEntity* player)
{
	int clss = 0;
	entprops->GetEntProp(player, Prop_Send, "m_iClass", clss);
	return clss;
}

bool csslib::IsInBuyZone(CBaseEntity* player)
{
	bool ret = false;
	entprops->GetEntPropBool(player, Prop_Send, "m_bInBuyZone", ret);
	return ret;
}

bool csslib::IsInBombPlantZone(CBaseEntity* player)
{
	bool ret = false;
	entprops->GetEntPropBool(player, Prop_Send, "m_bInBombZone", ret);
	return ret;
}

bool csslib::IsInHostageRescueZone(CBaseEntity* player)
{
	bool ret = false;
	entprops->GetEntPropBool(player, Prop_Send, "m_bInHostageRescueZone", ret);
	return ret;
}

bool csslib::HasNightVisionGoggles(CBaseEntity* player)
{
	bool ret = false;
	entprops->GetEntPropBool(player, Prop_Send, "m_bHasNightVision", ret);
	return ret;
}

bool csslib::HasHelmetEquipped(CBaseEntity* player)
{
	bool ret = false;
	entprops->GetEntPropBool(player, Prop_Send, "m_bHasHelmet", ret);
	return ret;
}

bool csslib::HasDefuser(CBaseEntity* player)
{
	bool ret = false;
	entprops->GetEntPropBool(player, Prop_Send, "m_bHasDefuser", ret);
	return ret;
}

bool csslib::IsDefusing(CBaseEntity* player)
{
	bool ret = false;
	entprops->GetEntPropBool(player, Prop_Send, "m_bIsDefusing", ret);
	return ret;
}

float csslib::GetC4TimeRemaining(CBaseEntity* c4)
{
	float time = 0.0f;
	entprops->GetEntPropFloat(c4, Prop_Send, "m_flC4Blow", time);
	return time - gpGlobals->curtime;
}

bool csslib::IsC4BeingDefused(CBaseEntity* c4)
{
	float time = 0.0f;
	entprops->GetEntPropFloat(c4, Prop_Send, "m_flDefuseCountDown", time);
	return time > 0.0f;
}

float csslib::GetC4DefuseTimeRemaining(CBaseEntity* c4)
{
	float time = 0.0f;
	entprops->GetEntPropFloat(c4, Prop_Send, "m_flDefuseCountDown", time);
	return time - gpGlobals->curtime;
}

bool csslib::IsHostageRescued(CBaseEntity* hostage)
{
	bool ret = false;
	entprops->GetEntPropBool(hostage, Prop_Send, "m_isRescued", ret);
	return ret;
}

CBaseEntity* csslib::GetHostageLeader(CBaseEntity* hostage)
{
	CBaseEntity* leader = nullptr;
	entprops->GetEntPropEnt(hostage, Prop_Send, "m_leader", nullptr, &leader);
	return leader;
}

bool csslib::IsInFreezeTime()
{
	bool ret = false;
	entprops->GameRules_GetPropBool("m_bFreezePeriod", ret);
	return ret;
}

bool csslib::MapHasBombTargets()
{
	bool ret = false;
	entprops->GameRules_GetPropBool("m_bMapHasBombTarget", ret);
	return ret;
}

bool csslib::MapHasHostageRescueZones()
{
	bool ret = false;
	entprops->GameRules_GetPropBool("m_bMapHasRescueZone", ret);
	return ret;
}

int csslib::GetNumberOfRemainingHostages()
{
	int val = 0;
	entprops->GameRules_GetProp("m_iHostagesRemaining", val);
	return val;
}

float csslib::GetRoundStartTime()
{
	float time = 0.0f;
	entprops->GameRules_GetPropFloat("m_fRoundStartTime", time);
	return time;
}

float csslib::GetRoundTimeRemaining()
{
	int roundtime = 0;
	
	if (entprops->GameRules_GetProp("m_iRoundTime", roundtime))
	{
		return gpGlobals->curtime - (GetRoundStartTime() + static_cast<float>(roundtime));
	}

	return std::numeric_limits<float>::max();
}

float csslib::GetFlashbangMaxDuration(CBaseEntity* player)
{
	float time = 0.0f;
	entprops->GetEntPropFloat(player, Prop_Send, "m_flFlashDuration", time);
	return time;
}

float csslib::GetFlashbangMaxAlpha(CBaseEntity* player)
{
	float time = 0.0f;
	entprops->GetEntPropFloat(player, Prop_Send, "m_flFlashMaxAlpha", time);
	return time;
}
