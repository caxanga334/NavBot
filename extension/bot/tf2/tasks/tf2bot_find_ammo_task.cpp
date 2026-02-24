#include NAVBOT_PCH_FILE
#include <limits>

#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <util/librandom.h>
#include <entities/tf2/tf_entities.h>
#include <sdkports/debugoverlay_shared.h>
#include <sdkports/sdk_traces.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <bot/tf2/tf2bot.h>
#include <bot/bot_shared_utils.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_area.h>
#include "tf2bot_find_ammo_task.h"

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

#undef min
#undef max
#undef clamp

#ifdef EXT_DEBUG
static ConVar convar_debug_ammo_search("sm_navbot_tf_debug_find_ammo", "0", FCVAR_GAMEDLL, "Enable find ammo debug");
#endif // EXT_DEBUG


class CTF2AmmoFilter : public UtilHelpers::IGenericFilter<CBaseEntity*>
{
public:
	CTF2AmmoFilter(CTF2Bot* tfbot)
	{
		bot = tfbot;
		myclass = tfbot->GetMyClassType();
		ammoarea = nullptr;
		teamindex = static_cast<int>(tfbot->GetMyTFTeam());
	}

	// Inherited via IGenericFilter
	bool IsSelected(CBaseEntity* object) override;

	CTF2Bot* bot;
	TeamFortress2::TFClassType myclass;
	CNavArea* ammoarea;
	int teamindex;
};

bool CTF2AmmoFilter::IsSelected(CBaseEntity* object)
{
	Vector position = UtilHelpers::getWorldSpaceCenter(object);
	position = trace::getground(position);
	this->ammoarea = TheNavMesh->GetNearestNavArea(position, CPath::PATH_GOAL_MAX_DISTANCE_TO_AREA * 2.0f, false, true, teamindex);

	if (this->ammoarea == nullptr)
	{
#ifdef EXT_DEBUG
		if (convar_debug_ammo_search.GetBool())
		{
			META_CONPRINTF("CTF2AmmoFilter::IsSelected entity <%s> at %g %g %g skipped! NULL Nav Area! \n",
				gamehelpers->GetEntityClassname(object), position.x, position.y, position.z);
		}
#endif

		return false; // no nav area
	}

	TeamFortress2::TFTeam team = tf2lib::GetEntityTFTeam(object);

	if (team >= TeamFortress2::TFTeam::TFTeam_Red && team != this->bot->GetMyTFTeam())
	{
		return false; // not for my team
	}

	tfentities::HTFBaseEntity baseentity(object);

	// func_regenerate entities have NODRAW but are valid
	if (UtilHelpers::FClassnameIs(object, "item_ammopack*") && baseentity.IsEffectActive(EF_NODRAW))
	{
#ifdef EXT_DEBUG
		if (convar_debug_ammo_search.GetBool())
		{
			META_CONPRINTF("CTF2AmmoFilter::IsSelected entity <%s> at %g %g %g skipped! EF_NODRAW! \n",
				gamehelpers->GetEntityClassname(object), position.x, position.y, position.z);
		}
#endif

		return false;
	}

	if (UtilHelpers::FClassnameIs(object, "obj_dispenser"))
	{
		tfentities::HObjectDispenser dispenser(object);

		if (dispenser.IsDisabled() || dispenser.IsSapped() || dispenser.IsBeingCarried() || dispenser.IsPlacing() || dispenser.IsBuilding())
		{
			return false;
		}

		// Engineers: skip dispensers unless they have at least 200 metal
		if (this->myclass == TeamFortress2::TFClass_Engineer && dispenser.GetStoredMetal() < TeamFortress2::TF_DEFAULT_MAX_METAL)
		{
			return false;
		}
	}

	return true;
}

bool CTF2BotFindAmmoTask::IsPossible(CTF2Bot* bot, CBaseEntity** source)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CTF2BotFindAmmoTask::IsPossible", "NavBot");
#endif // EXT_VPROF_ENABLED

	const float maxrange = CTeamFortress2Mod::GetTF2Mod()->GetModSettings()->GetCollectItemMaxDistance();
	CTF2AmmoFilter filter(bot);
	botsharedutils::IsReachableAreas collector(bot, maxrange);

	CBaseEntity* best = nullptr;
	float smallest_dist = std::numeric_limits<float>::max();
	bool is_resupply_locker = true;

	collector.Execute();

	auto func = [&filter, &best, &smallest_dist, &collector, &is_resupply_locker](int index, edict_t* edict, CBaseEntity* entity) {
		if (entity)
		{
			if (filter.IsSelected(entity))
			{
				float cost = 0.0f;

#ifdef EXT_DEBUG
				if (convar_debug_ammo_search.GetBool())
				{
					float dcost = -1.0f;
					const bool reachable = collector.IsReachable(filter.ammoarea, &dcost);
					META_CONPRINTF("CTF2BotFindAmmoTask::IsPossible ENTITY <%s> IS %s WITH A COST OF %g\n",
						gamehelpers->GetEntityClassname(entity), reachable ? "REACHABLE" : "UNREACHABLE", dcost);
				}
#endif

				if (collector.IsReachable(filter.ammoarea, &cost))
				{
					if (is_resupply_locker)
					{
						cost *= 0.6f; // artificial cost reduction for resupply lockers so bots prefer using them
					}

					if (cost < smallest_dist)
					{
						smallest_dist = cost;
						best = entity;
					}
				}
			}
		}

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("func_regenerate", func);
	is_resupply_locker = false;
	UtilHelpers::ForEachEntityOfClassname("item_ammopack*", func);
	UtilHelpers::ForEachEntityOfClassname("obj_dispenser", func);
	UtilHelpers::ForEachEntityOfClassname("tf_ammo_pack", func);

	if (!best)
	{
		return false;
	}

	*source = best;
	return true;
}

CTF2BotFindAmmoTask::CTF2BotFindAmmoTask(CBaseEntity* entity) :
	m_sourceentity(entity)
{
	if (UtilHelpers::FClassnameIs(entity, "obj_dispenser"))
	{
		m_type = AmmoSource::DISPENSER;
	}
	else if (UtilHelpers::FClassnameIs(entity, "func_regenerate"))
	{
		m_type = AmmoSource::RESUPPLY;
	}
	else
	{
		m_type = AmmoSource::AMMOPACK;
	}

	m_isDroppedAmmo = UtilHelpers::FClassnameIs(entity, "tf_ammo_pack");
	m_metalLimit = TeamFortress2::TF_DEFAULT_MAX_METAL;
}

CTF2BotFindAmmoTask::CTF2BotFindAmmoTask(CBaseEntity* entity, int maxmetal) :
	m_sourceentity(entity)
{
	if (UtilHelpers::FClassnameIs(entity, "obj_dispenser"))
	{
		m_type = AmmoSource::DISPENSER;
	}
	else if (UtilHelpers::FClassnameIs(entity, "func_regenerate"))
	{
		m_type = AmmoSource::RESUPPLY;
	}
	else
	{
		m_type = AmmoSource::AMMOPACK;
	}

	m_isDroppedAmmo = UtilHelpers::FClassnameIs(entity, "tf_ammo_pack");
	m_metalLimit = maxmetal;
}

TaskResult<CTF2Bot> CTF2BotFindAmmoTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	if (!IsSourceStillValid(bot))
	{
		return Done("Ammo Source is invalid!");
	}

	float range = bot->GetRangeTo(UtilHelpers::getWorldSpaceCenter(m_sourceentity.Get()));
	float time = range / (bot->GetMaxSpeed() * 0.25f);
	m_failsafetimer.Start(time + 8.0f);

	TeamFortress2::TFClassType myclass = bot->GetMyClassType();

	// Equip melee to make sniper bots unscope, they will switch back to the sniper/smg if an enemy is visible
	if (myclass == TeamFortress2::TFClassType::TFClass_Sniper)
	{
		CBaseEntity* weapon = bot->GetWeaponOfSlot(static_cast<int>(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Melee));

		if (weapon)
		{
			bot->SelectWeapon(weapon);
		}
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotFindAmmoTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (!bot->GetInventoryInterface()->IsAmmoLow(false))
	{
		return Done("Got ammo!");
	}

	if (!IsSourceStillValid(bot))
	{
		return Done("Ammo Source is invalid!");
	}

	if (m_failsafetimer.IsElapsed())
	{
		return Done("Timed out!");
	}

	if (bot->GetMyClassType() == TeamFortress2::TFClass_Engineer && bot->GetAmmoOfIndex(TeamFortress2::TF_AMMO_METAL) >= m_metalLimit)
	{
		return Done("Ammo collected!");
	}

	CBaseEntity* source = m_sourceentity.Get();
	Vector goal = UtilHelpers::getWorldSpaceCenter(source);

	if (m_nav.NeedsRepath())
	{
		m_nav.StartRepathTimer();

		CTF2BotPathCost cost(bot);
		if (!m_nav.ComputePathToPosition(bot, goal, cost))
		{
			return Done("Failed to build a path to the ammo source!");
		}
	}

	// if the bot is this close to the dispenser, stop moving
	static constexpr auto DISPENSER_TOUCH_RANGE = 64.0f;

	if (m_type == AmmoSource::DISPENSER && bot->GetRangeTo(goal) < DISPENSER_TOUCH_RANGE)
	{
		return Continue();
	}

	m_nav.Update(bot);

	return Continue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotFindAmmoTask::OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason)
{
	// don't clear stuck status here, can cause bots to get stuck forever
	m_nav.Invalidate();
	m_nav.ForceRepath();
	return TryContinue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotFindAmmoTask::OnMoveToSuccess(CTF2Bot* bot, CPath* path)
{
	return TryContinue();
}

bool CTF2BotFindAmmoTask::IsSourceStillValid(CTF2Bot* me)
{
	auto edict = gamehelpers->GetHandleEntity(m_sourceentity);

	if (!edict)
		return false; // source entity no longer exists

	if (m_isDroppedAmmo)
	{
		return true;
	}

	switch (m_type)
	{
	case CTF2BotFindAmmoTask::AmmoSource::AMMOPACK:
	{
		tfentities::HTFBaseEntity entity(edict);

		if (entity.IsEffectActive(EF_NODRAW))
			return false;

		if (entity.IsDisabled())
			return false;

		break;
	}
	case CTF2BotFindAmmoTask::AmmoSource::RESUPPLY:
	{

		tfentities::HFuncRegenerate regen(edict);

		if (regen.IsDisabled())
			return false;

		if (regen.GetTFTeam() != TeamFortress2::TFTeam_Unassigned && regen.GetTFTeam() != me->GetMyTFTeam())
			return false;

		break;
	}
	case CTF2BotFindAmmoTask::AmmoSource::DISPENSER:
	{
		tfentities::HBaseObject object(edict);

		if (object.IsSapped() || object.IsDisabled() || object.IsPlacing() || object.IsBuilding() || object.IsBeingCarried() || object.IsRedeploying())
			return false;

		if (object.GetTFTeam() != me->GetMyTFTeam())
			return false;

		break;
	}
	default:
		break;
	}

	return true;
}
