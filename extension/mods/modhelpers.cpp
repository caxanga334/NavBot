#include NAVBOT_PCH_FILE
#include "modhelpers.h"

std::unique_ptr<IModHelpers> modhelpers{ nullptr };

IModHelpers::IModHelpers()
{
}

IModHelpers::~IModHelpers()
{
}

void IModHelpers::SetInstance(IModHelpers* mh)
{
	modhelpers.reset(mh);
}

int IModHelpers::GetEntityTeamNumber(CBaseEntity* entity)
{
	int iTeamNum = 0;
	entprops->GetEntProp(entity, Prop_Data, "m_iTeamNum", iTeamNum);
	return iTeamNum;
}

bool IModHelpers::IsPlayer(CBaseEntity* entity)
{
	return UtilHelpers::IsPlayerIndex(reinterpret_cast<IServerEntity*>(entity)->GetRefEHandle().GetEntryIndex());
}

bool IModHelpers::IsAlive(CBaseEntity* entity)
{
	int lifestate = LIFE_ALIVE;
	entprops->GetEntProp(entity, Prop_Data, "m_lifeState", lifestate);

	if (IsPlayer(entity))
	{
		bool deadflag = false;

		if (entprops->GetEntPropBool(entity, Prop_Send, "deadflag", deadflag))
		{
			// a player is alive if their lifestate is alive and the deadflag is false.
			return (lifestate == LIFE_ALIVE && !deadflag);
		}
	}

	return lifestate == LIFE_ALIVE;
}

bool IModHelpers::IsCombatCharacter(CBaseEntity* entity)
{
	// In most games, m_hActiveWeapon is a member of CBaseCombatCharacter
	return entprops->HasEntProp(entity, Prop_Data, "m_hActiveWeapon");
}

int IModHelpers::GetEntityWaterLevel(CBaseEntity* entity)
{
#ifdef EXT_DEBUG
	if (!entprops->HasEntProp(entity, Prop_Data, "m_nWaterLevel"))
	{
		META_CONPRINTF("IModHelpers::GetEntityWaterLevel: Entity %s doesn't have CBaseEntity::m_nWaterLevel property!\n", UtilHelpers::textformat::FormatEntity(entity));
	}
#endif // EXT_DEBUG

	int level = 0;
	entprops->GetEntProp(entity, Prop_Data, "m_nWaterLevel", level);
	return level;
}

