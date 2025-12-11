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
#include <bot/tasks_shared/bot_shared_take_cover_from_spot.h>
#include <bot/tasks_shared/bot_shared_use_self_heal_item.h>
#include "tf2bot_find_health_task.h"

#undef min
#undef max
#undef clamp

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

class CTF2HealthFilter : public UtilHelpers::IGenericFilter<CBaseEntity*>
{
public:
	CTF2HealthFilter(CTF2Bot* tfbot)
	{
		bot = tfbot;
		healtharea = nullptr;
	}

	// Inherited via IGenericFilter
	bool IsSelected(CBaseEntity* object) override;

	CTF2Bot* bot;
	CNavArea* healtharea;
};

bool CTF2HealthFilter::IsSelected(CBaseEntity* object)
{
	Vector position = UtilHelpers::getWorldSpaceCenter(object);
	position = trace::getground(position);
	this->healtharea = TheNavMesh->GetNearestNavArea(position, CPath::PATH_GOAL_MAX_DISTANCE_TO_AREA * 2.0f);

	if (this->healtharea == nullptr)
	{
		return false; // no nav area
	}

	TeamFortress2::TFTeam team = tf2lib::GetEntityTFTeam(object);

	if (team >= TeamFortress2::TFTeam::TFTeam_Red && team != this->bot->GetMyTFTeam())
	{
		return false; // not for my team
	}

	tfentities::HTFBaseEntity baseentity(object);

	if (UtilHelpers::FClassnameIs(object, "item_healthkit*") && baseentity.IsEffectActive(EF_NODRAW))
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
	}

	return true;
}

CTF2BotFindHealthTask::CTF2BotFindHealthTask(CBaseEntity* source) :
	m_sourceentity(source)
{
	if (UtilHelpers::FClassnameIs(source, "obj_dispenser"))
	{
		m_type = HealthSource::DISPENSER;
	}
	else if (UtilHelpers::FClassnameIs(source, "func_regenerate"))
	{
		m_type = HealthSource::RESUPPLY;
	}
	else
	{
		m_type = HealthSource::HEALTHPACK;
	}

	m_heathlValueLastUpdate = 0;
	m_dontMove = false;
}

bool CTF2BotFindHealthTask::IsPossible(CTF2Bot* bot, CBaseEntity** source)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CTF2BotFindHealthTask::IsPossible", "NavBot");
#endif // EXT_VPROF_ENABLED

	if (bot->GetMyClassType() == TeamFortress2::TFClassType::TFClass_Heavy)
	{
		CBaseEntity* weapon = bot->GetWeaponOfSlot(static_cast<int>(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Secondary));

		if (weapon && UtilHelpers::FClassnameIs(weapon, "tf_weapon_lunchbox"))
		{
			const CBotWeapon* botweapon = bot->GetInventoryInterface()->GetWeaponOfEntity(weapon);

			if (botweapon && !botweapon->IsOutOfAmmo(bot))
			{
				int index = 0;
				entprops->GetEntProp(UtilHelpers::IndexOfEntity(weapon), Prop_Send, "m_iItemDefinitionIndex", index);

				// 311 is The Buffalo Steak Sandvich
				if (index != 311)
				{
					*source = weapon;
					return true;
				}
			}
		}
	}

	const float maxrange = CTeamFortress2Mod::GetTF2Mod()->GetModSettings()->GetCollectItemMaxDistance();
	CTF2HealthFilter filter(bot);
	botsharedutils::IsReachableAreas collector(bot, maxrange);

	CBaseEntity* best = nullptr;
	float smallest_dist = std::numeric_limits<float>::max();

	collector.Execute();

	auto functor = [&filter, &best, &smallest_dist, &collector](int index, edict_t* edict, CBaseEntity* entity) {
		if (entity)
		{
			if (filter.IsSelected(entity))
			{
				float cost = 0.0f;

				if (collector.IsReachable(filter.healtharea, &cost))
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

	UtilHelpers::ForEachEntityOfClassname("func_regenerate", functor);
	UtilHelpers::ForEachEntityOfClassname("item_healthkit*", functor);
	UtilHelpers::ForEachEntityOfClassname("obj_dispenser", functor);

	if (!best)
	{
		return false;
	}

	*source = best;
	return true;
}

TaskResult<CTF2Bot> CTF2BotFindHealthTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	CBaseEntity* entity = m_sourceentity.Get();

	if (!entity)
	{
		return Done("Health source entity is NULL!");
	}

	if (UtilHelpers::FClassnameIs(entity, "tf_weapon_lunchbox"))
	{
		// sandvich, eat it instead of collect.

		const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(true);

		if (threat)
		{
			AITask<CTF2Bot>* task = new CBotSharedUseSelfHealItemTask<CTF2Bot>(entity, 10.0f, true);
			return SwitchTo(new CBotSharedTakeCoverFromSpotTask<CTF2Bot, CTF2BotPathCost>(bot, threat->GetLastKnownPosition(), 256.0f, true, true, 4096.0f, task), "Taking cover to heal!");
		}

		// no visible threat
		return SwitchTo(new CBotSharedUseSelfHealItemTask<CTF2Bot>(entity, 10.0f, true), "Using self heal item!");
	}

	CTF2BotPathCost cost(bot);
	if (!m_nav.ComputePathToPosition(bot, UtilHelpers::getWorldSpaceCenter(entity), cost))
	{
		return Done("Failed to build a path to the Health source!");
	}

	float range = bot->GetRangeTo(UtilHelpers::getWorldSpaceCenter(m_sourceentity.Get()));
	float time = range / (bot->GetMaxSpeed() * 0.25f);
	m_failsafetimer.Start(time + 8.0f);

	if (tf2lib::GetNumberOfPlayersAsClass(TeamFortress2::TFClassType::TFClass_Medic, bot->GetMyTFTeam()) > 0)
	{
		bot->SendVoiceCommand(TeamFortress2::VoiceCommandsID::VC_MEDIC);
	}

	m_heathlValueLastUpdate = bot->GetHealth();
	m_healthUpdateTimer.Start(1.0f);

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotFindHealthTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (bot->GetHealthPercentage() >= 0.98f)
	{
		return Done("I'm at full health!");
	}

	// Stop going for health if a medic deploys an uber
	if (tf2lib::IsPlayerInvulnerable(bot->GetIndex()))
	{
		return Done("I'm invulnerable!");
	}

	if (!IsSourceStillValid(bot))
		return Done("Health Source is invalid!");

	if (m_failsafetimer.IsElapsed())
	{
		return Done("Timed out!");
	}

	CBaseEntity* entity = m_sourceentity.Get();
	Vector pos = UtilHelpers::getWorldSpaceCenter(entity);

	// if the bot is this close to the dispenser, stop moving
	static constexpr auto DISPENSER_TOUCH_RANGE = 64.0f;

	if (m_type == HealthSource::DISPENSER && bot->GetRangeTo(pos) < DISPENSER_TOUCH_RANGE)
	{
		return Continue();
	}

	if (m_healthUpdateTimer.IsElapsed())
	{
		m_healthUpdateTimer.Start(1.0f);

		const int current = bot->GetHealth();
		const int delta = (m_heathlValueLastUpdate - current);
		m_heathlValueLastUpdate = current;

		// if the delta is negative, the bot is gaining health
		if (delta < 0)
		{
			const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat();

			if (!threat || !threat->IsVisibleNow())
			{
				m_dontMove = true;
			}
			else
			{
				m_dontMove = false;
			}
		}
		else
		{
			m_dontMove = false;
		}
	}

	if (m_dontMove)
	{
		return Continue();
	}

	CTF2BotPathCost cost(bot);
	m_nav.Update(bot, pos, cost);

	return Continue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotFindHealthTask::OnMoveToSuccess(CTF2Bot* bot, CPath* path)
{
	if (m_type == HealthSource::DISPENSER)
	{
		m_failsafetimer.StartRandom(15.0f, 25.0f);
	}

	return TryContinue();
}

bool CTF2BotFindHealthTask::IsSourceStillValid(CTF2Bot* me)
{
	auto edict = gamehelpers->GetHandleEntity(m_sourceentity);

	if (!edict)
		return false; // source entity no longer exists

	switch (m_type)
	{
	case CTF2BotFindHealthTask::HealthSource::HEALTHPACK:
	{
		tfentities::HTFBaseEntity entity(edict);

		if (entity.IsEffectActive(EF_NODRAW) || entity.IsDisabled())
			return false;

		break;
	}
	case CTF2BotFindHealthTask::HealthSource::RESUPPLY:
	{
		tfentities::HFuncRegenerate regen(edict);

		if (regen.IsDisabled())
			return false;

		if (regen.GetTFTeam() != TeamFortress2::TFTeam_Unassigned && regen.GetTFTeam() != me->GetMyTFTeam())
			return false;

		break;
	}
	case CTF2BotFindHealthTask::HealthSource::DISPENSER:
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