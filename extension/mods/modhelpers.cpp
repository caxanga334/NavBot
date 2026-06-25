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

int IModHelpers::GetEntityTeamNumber(CBaseEntity* entity) const
{
	int iTeamNum = 0;
	entprops->GetEntProp(entity, Prop_Data, "m_iTeamNum", iTeamNum);
	return iTeamNum;
}

bool IModHelpers::IsPlayer(CBaseEntity* entity) const
{
	return UtilHelpers::IsPlayerIndex(reinterpret_cast<IServerEntity*>(entity)->GetRefEHandle().GetEntryIndex());
}

bool IModHelpers::IsAlive(CBaseEntity* entity) const
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

bool IModHelpers::IsCombatCharacter(CBaseEntity* entity) const
{
	// In most games, m_hActiveWeapon is a member of CBaseCombatCharacter
	return entprops->HasEntProp(entity, Prop_Data, "m_hActiveWeapon");
}

int IModHelpers::GetEntityWaterLevel(CBaseEntity* entity) const
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

bool IModHelpers::IsEntityVisible(CBaseEntity* entity) const
{
	if (entityprops::IsEffectActiveOnEntity(entity, EF_NODRAW))
	{
		return false;
	}

	return true;
}

bool IModHelpers::CanPlayerPickUpItem(CBaseEntity* item, CBaseEntity* player) const
{
	// basic team check, should work for most team based games
	int itemTeam = GetEntityTeamNumber(item);

	if (itemTeam > TEAM_SPECTATOR)
	{
		return itemTeam == GetEntityTeamNumber(player);
	}

	// Generally items waiting for their respawn time have NODRAW set.
	if (entityprops::IsEffectActiveOnEntity(item, EF_NODRAW))
	{
		return false;
	}

	// No weapon ownership check, leave that to game specific implementations
	// The bot will generally ignore it anyways

	return true;
}

bool IModHelpers::IsPlayableTeam(int teamNum) const
{
	switch (teamNum)
	{
	case TEAM_UNASSIGNED:
		[[fallthrough]];
	case TEAM_SPECTATOR:
		return false;
	default:
		return true;
	}
}

int IModHelpers::GetOpposingTeamIndex(int teamNum) const
{
	// Generally, mods uses 2 as team one and 3 as team two
	switch (teamNum)
	{
	case 3:
		return 2;
	case 2:
		return 3;
	default:
		return NO_OPPOSING_TEAM;
	}
}

