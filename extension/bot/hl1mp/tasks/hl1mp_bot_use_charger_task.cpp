#include NAVBOT_PCH_FILE
#include <mods/basemod.h>
#include <navmesh/nav_mesh.h>
#include <bot/hl1mp/hl1mp_bot.h>
#include <bot/bot_shared_utils.h>
#include <bot/interfaces/path/chasenavigator.h>
#include "hl1mp_bot_use_charger_task.h"

class CHL1MPChargerFilter : public botsharedutils::search::SearchReachableEntities<CHL1MPBot, CNavArea>
{
public:
	CHL1MPChargerFilter(CHL1MPBot* bot) :
		botsharedutils::search::SearchReachableEntities<CHL1MPBot, CNavArea>(bot, extmanager->GetMod()->GetModSettings()->GetCollectItemMaxDistance())
	{
		SetCheckGround(false);
	}

protected:
	Vector GetEntityPosition(CBaseEntity* entity) const override
	{
		return trace::getground(UtilHelpers::getWorldSpaceCenter(entity));
	}

	bool IsEntityValid(CBaseEntity* entity, CNavArea* area) override
	{
		if (botsharedutils::search::SearchReachableEntities<CHL1MPBot, CNavArea>::IsEntityValid(entity, area))
		{
			int juice = 0;
			entprops->GetEntProp(entity, Prop_Data, "m_iJuice", juice);

			if (juice <= 0)
			{
				return false;
			}

			return true;
		}

		return false;
	}
};

class CTraceFilterIgnoreChargers : public trace::CTraceFilterSimple
{
public:
	CTraceFilterIgnoreChargers(CHL1MPBot* bot) :
		trace::CTraceFilterSimple(bot->GetEntity(), COLLISION_GROUP_NONE)
	{
	}

	bool ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask) override
	{
		if (trace::CTraceFilterSimple::ShouldHitEntity(pHandleEntity, contentsMask))
		{
			CBaseEntity* pEntity = trace::EntityFromEntityHandle(pHandleEntity);

			if (UtilHelpers::FClassnameIs(pEntity, "func_healthcharger") || UtilHelpers::FClassnameIs(pEntity, "func_recharge"))
			{
				return false;
			}

			return true;
		}

		return false;
	}
};

bool CHL1MPBotUseChargerTask::IsPossible(CHL1MPBot* bot, ChargerType type, CBaseEntity** entity)
{
	CHL1MPChargerFilter filter{ bot };

	switch (type)
	{
	case ChargerType::HEALTH_CHARGER:
		filter.AddSearchPattern("func_healthcharger");
		break;
	case ChargerType::ARMOR_CHARGER:
		filter.AddSearchPattern("func_recharge");
		break;
	default: return false;
	}

	filter.DoSearch();
	
	*entity = filter.SelectNearest();
	return *entity != nullptr;
}

TaskResult<CHL1MPBot> CHL1MPBotUseChargerTask::OnTaskStart(CHL1MPBot* bot, AITask<CHL1MPBot>* pastTask)
{
	if (!IsChargerValid())
	{
		return Done("Charger is invalid!");
	}

#if 0 // problematic
	CHL1MPBotPathCost cost{ bot, RouteType::FASTEST_ROUTE };

	if (!m_nav.ComputePathToPosition(bot, UtilHelpers::getEntityOrigin(m_entity.Get()), cost))
	{
		return Done("Failed to build a complete path to the charger entity!");
	}
#endif

	return Continue();
}

TaskResult<CHL1MPBot> CHL1MPBotUseChargerTask::OnTaskUpdate(CHL1MPBot* bot)
{
	if (!IsChargerValid())
	{
		return Done("Charger is invalid!");
	}

	switch (m_type)
	{
	case ChargerType::HEALTH_CHARGER:
	{
		if (bot->GetHealth() >= bot->GetMaxHealth())
		{
			return Done("I am at full health!");
		}

		break;
	}
	case ChargerType::ARMOR_CHARGER:
	{
		if (bot->GetArmorAmount() >= 100)
		{
			return Done("I am at full armor!");
		}

		break;
	}
	default: break;
	}

	CBaseEntity* charger = m_entity.Get();
	Vector center = UtilHelpers::getWorldSpaceCenter(charger);
	Vector eyepos = bot->GetEyeOrigin();

	if ((eyepos - center).IsLengthLessThan(CBaseExtPlayer::PLAYER_USE_RADIUS))
	{
		CTraceFilterIgnoreChargers filter{ bot };
		trace_t tr;
		trace::line(eyepos, center, MASK_SOLID, &filter, tr);

		if (tr.fraction >= 1.0f || tr.m_pEnt == charger)
		{
			bot->GetControlInterface()->AimAt(center, IPlayerController::LOOK_CRITICAL, 1.0f, "Looking at charger to use it!");

			if (bot->GetControlInterface()->IsAimOnTarget())
			{
				bot->GetControlInterface()->PressUseButton(0.5f);
			}
		}
	}
	else
	{
		if (!m_nav.IsValid() || m_nav.NeedsRepath())
		{
			CHL1MPBotPathCost cost{ bot, RouteType::FASTEST_ROUTE };
			m_nav.StartRepathTimer();
			m_nav.ComputePathToPosition(bot, center, cost);
		}

		m_nav.Update(bot);
	}

	return Continue();
}

TaskEventResponseResult<CHL1MPBot> CHL1MPBotUseChargerTask::OnStuck(CHL1MPBot* bot)
{
	if (IsChargerValid())
	{
		CBaseEntity* charger = m_entity.Get();
		Vector center = UtilHelpers::getWorldSpaceCenter(charger);
		CHL1MPBotPathCost cost{ bot, RouteType::FASTEST_ROUTE };

		if (!m_nav.ComputePathToPosition(bot, center, cost))
		{
			return TryDone(PRIORITY_HIGH, "No path to charger!");
		}
	}

	return TryContinue();
}

TaskEventResponseResult<CHL1MPBot> CHL1MPBotUseChargerTask::OnMoveToFailure(CHL1MPBot* bot, CPath* path, IEventListener::MovementFailureType reason)
{
	if (IsChargerValid())
	{
		CBaseEntity* charger = m_entity.Get();
		Vector center = UtilHelpers::getWorldSpaceCenter(charger);
		CHL1MPBotPathCost cost{ bot, RouteType::FASTEST_ROUTE };

		if (!m_nav.ComputePathToPosition(bot, center, cost))
		{
			return TryDone(PRIORITY_HIGH, "No path to charger!");
		}
	}

	return TryContinue();
}

bool CHL1MPBotUseChargerTask::IsChargerValid() const
{
	CBaseEntity* charger = m_entity.Get();

	if (!charger) { return false; }

	int juice = 0;
	entprops->GetEntProp(charger, Prop_Data, "m_iJuice", juice);

	if (juice <= 0)
	{
		return false;
	}

	return true;
}
