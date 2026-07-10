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

bool IModHelpers::PassesFilterImpl(CBaseEntity* filter, CBaseEntity* caller, CBaseEntity* activator) const
{
#ifdef EXT_DEBUG
	if (!sdkcalls->IsPassesFilterImplAvailable())
	{
		smutils->LogError(myself, "IModHelpers::PassesFilterImpl called but SDKCall isn't available!", UtilHelpers::textformat::FormatEntity(filter));
	}
#endif // EXT_DEBUG

	bool negated = false;
	
	if (!entprops->GetEntPropBool(filter, Prop_Data, "m_bNegated", negated))
	{
		/* 
		* Some level designed assigns a non filter entity as a damage filter.
		* The game doesn't crash because it performs a dynamic_cast to CBaseFilter.
		* NavBot however can't do dynamic_cast for game classes so we have to check using other methods.
		* m_bNegated is a member of CBaseFilter, if the entity doesn't have it, good chance it's not a CBaseFilter entity.
		*/

#ifdef EXT_DEBUG
		smutils->LogError(myself, "IModHelpers::PassesFilterImpl entity %s doesn't have m_bNegated datamap!", UtilHelpers::textformat::FormatEntity(filter));
#endif // EXT_DEBUG
		return true;
	}

	bool base = sdkcalls->CBaseFilter_PassesFilterImpl(filter, caller, activator);
	return negated ? !base : base;
}

bool IModHelpers::IsUseObstructed(CBaseEntity* player, CBaseEntity* entity, CBaseEntity** obstruction) const
{
	// see CBasePlayer::FindUseEntity()
	// https://github.com/ValveSoftware/source-sdk-2013/blob/88fa198fba3fb85d46d4c95018254693fdc3af0a/src/game/shared/baseplayer_shared.cpp#L1067

	constexpr int useableContents = MASK_SOLID | CONTENTS_DEBRIS | CONTENTS_PLAYERCLIP;

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

bool IModHelpers::IsEntityDamageable(CBaseEntity* entity, const int maxhealth) const
{
	int takedamage = 0;

	if (entprops->GetEntProp(entity, Prop_Data, "m_takedamage", takedamage))
	{
		switch (takedamage)
		{
		case DAMAGE_NO:
			[[fallthrough]];
		case DAMAGE_EVENTS_ONLY:
		{
			return false; // entity doesn't take damage
		}
		default:
			break;
		}
	}

	int health = 0;

	if (entprops->GetEntProp(entity, Prop_Data, "m_iHealth", health))
	{
		if (health > maxhealth)
		{
			return false; // too much health
		}
	}

	return true;
}

bool IModHelpers::IsEntityDamageableBy(CBaseEntity* entity, CBaseEntity* attacker) const
{
	CBaseEntity* damageFilter = nullptr;

	if (entprops->GetEntPropEnt(entity, Prop_Data, "m_hDamageFilter", nullptr, &damageFilter))
	{
		if (damageFilter)
		{
			if (sdkcalls->IsPassesFilterImplAvailable())
			{
				if (!modhelpers->PassesFilterImpl(damageFilter, nullptr, attacker))
				{
					// The attacker does not passes the damage filter.
					return false;
				}
			}
			else
			{
				// The entity has a damage filter and the CBaseFilter::PassesFilterImpl SDKCall isn't available.
				// Consider as not damageable.
				return false;
			}
		}
	}

	return true;
}

bool IModHelpers::IsEntityBreakable(CBaseEntity* entity) const
{
	auto classname = gamehelpers->GetEntityClassname(entity);

	if (strncmp(classname, "func_breakable", 14) == 0)
	{
		return true;
	}

	if (strncmp(classname, "func_breakable_surf", 19) == 0)
	{
		return true;
	}

	if (strncmp(classname, "func_physbox", 12) == 0)
	{
		return true;
	}

	if (UtilHelpers::StringMatchesPattern(classname, "prop_phys*", 0))
	{
		return true;
	}

	return false;
}

bool IModHelpers::IsBreakableBroken(CBaseEntity* entity) const
{
	// breakable surfaces entities aren't deleted when broken.
	if (UtilHelpers::FClassnameIs(entity, "func_breakable_surf"))
	{
		bool* bBroken = entprops->GetPointerToEntData<bool>(entity, Prop_Send, "m_bIsBroken");

		if (bBroken != nullptr)
		{
			return *bBroken;
		}
	}

	return false;
}

bool IModHelpers::IsBreakableExplosive(CBaseEntity* entity) const
{
	if (UtilHelpers::FClassnameIs(entity, "prop_phys*"))
	{
		float dmg = -1.0f;
		entprops->GetEntPropFloat(entity, Prop_Data, "m_explodeDamage", dmg);
		
		if (dmg >= 0.1f)
		{
			return true;
		}
	}

	return false;
}
