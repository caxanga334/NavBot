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

class CTF2AmmoFilter : public UtilHelpers::IGenericFilter<CBaseEntity*>
{
public:
	CTF2AmmoFilter(CTF2Bot* tfbot)
	{
		bot = tfbot;
		myclass = tfbot->GetMyClassType();
		ammoarea = nullptr;
	}

	// Inherited via IGenericFilter
	bool IsSelected(CBaseEntity* object) override;

	CTF2Bot* bot;
	TeamFortress2::TFClassType myclass;
	CNavArea* ammoarea;
};

bool CTF2AmmoFilter::IsSelected(CBaseEntity* object)
{
	Vector position = UtilHelpers::getWorldSpaceCenter(object);
	position = trace::getground(position);
	this->ammoarea = TheNavMesh->GetNearestNavArea(position, CPath::PATH_GOAL_MAX_DISTANCE_TO_AREA);

	if (this->ammoarea == nullptr)
	{
		return false; // no nav area
	}

	TeamFortress2::TFTeam team = tf2lib::GetEntityTFTeam(object);

	if (team >= TeamFortress2::TFTeam::TFTeam_Red && team != this->bot->GetMyTFTeam())
	{
		return false; // not for my team
	}

	tfentities::HTFBaseEntity baseentity(object);

	if (baseentity.IsEffectActive(EF_NODRAW))
	{
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

	collector.Execute();

	auto func = [&filter, &best, &smallest_dist, &collector](int index, edict_t* edict, CBaseEntity* entity) {
		if (entity)
		{
			if (filter.IsSelected(entity))
			{
				float cost = 0.0f;

				if (collector.IsReachable(filter.ammoarea, &cost))
				{
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

	if (m_repathtimer.IsElapsed())
	{
		m_repathtimer.Start(2.0f);
		

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
	m_repathtimer.Invalidate(); // force repath
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
