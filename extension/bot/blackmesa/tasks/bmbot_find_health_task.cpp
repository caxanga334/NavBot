#include NAVBOT_PCH_FILE
#include <mods/blackmesa/blackmesadm_mod.h>
#include <mods/blackmesa/nav/bm_nav_mesh.h>
#include <bot/blackmesa/bmbot.h>
#include <bot/bot_shared_utils.h>
#include "bmbot_find_health_task.h"

class CBlackMesaHealthCollector final : public botsharedutils::search::SearchReachableEntities<CBlackMesaBot, CNavArea>
{
public:
	CBlackMesaHealthCollector(CBlackMesaBot* bot) :
		botsharedutils::search::SearchReachableEntities<CBlackMesaBot, CNavArea>(bot, CBlackMesaDeathmatchMod::GetBMMod()->GetModSettings()->GetCollectItemMaxDistance())
	{
		SetCheckCanPickup(true); // query the behavior to see if we should pick up items
		AddSearchPattern("item_healthkit");
		AddSearchPattern("item_healthcharger");
	}

private:
	bool IsEntityValid(CBaseEntity* entity, CNavArea* area) final
	{
		if (botsharedutils::search::SearchReachableEntities<CBlackMesaBot, CNavArea>::IsEntityValid(entity, area))
		{
			if (UtilHelpers::FClassnameIs(entity, "item_healthcharger"))
			{
				bool chargerIsOn = false;
				entprops->GetEntPropBool(entity, Prop_Send, "m_bOn", chargerIsOn);

				// skip empty chargers
				if (!chargerIsOn)
				{
					return false;
				}
			}

			return true;
		}

		return false;
	}
};

bool CBlackMesaBotFindHealthTask::IsPossible(CBlackMesaBot* bot, CBaseEntity** item)
{
	if (bot->GetHealthState() == CBaseBot::HealthState::HEALTH_OK)
	{
		return false;
	}
	CBlackMesaHealthCollector collector(bot);
	collector.DoSearch();
	CBaseEntity* entity = collector.SelectNearest(); // always use nearest for health

	if (!entity)
	{
		return false;
	}

	*item = entity;
	return true;
}

TaskResult<CBlackMesaBot> CBlackMesaBotFindHealthTask::OnTaskStart(CBlackMesaBot* bot, AITask<CBlackMesaBot>* pastTask)
{
	CBaseEntity* healthSource = m_healthSource.Get();

	if (!healthSource)
	{
		return Done("NULL entity!");
	}

	if (!IsHealthSourceValid())
	{
		return Done("Health source is invalid!");
	}

	return Continue();
}

TaskResult<CBlackMesaBot> CBlackMesaBotFindHealthTask::OnTaskUpdate(CBlackMesaBot* bot)
{
	if (bot->GetHealthPercentage() >= 0.99f)
	{
		return Done("I'm at full health!");
	}

	if (m_timeout.IsElapsed())
	{
		return Done("Timeout timer expired!");
	}

	if (!IsHealthSourceValid())
	{
		return Done("Health source entity is invalid!");
	}

	float moveToRange = m_isCharger ? 70.0f : 24.0f;

	if (bot->GetRangeTo(m_goal) > moveToRange)
	{
		if (m_nav.NeedsRepath())
		{
			m_nav.StartRepathTimer();
			CBlackMesaBotPathCost cost(bot);

			if (!m_nav.ComputePathToPosition(bot, m_goal, cost))
			{
				return Done("No route to health source!");
			}
		}

		m_nav.Update(bot);
	}

	if (m_isCharger && bot->GetRangeTo(m_goal) < CBaseExtPlayer::PLAYER_USE_RADIUS)
	{
		bot->GetControlInterface()->AimAt(m_goal, IPlayerController::LOOK_USE, 0.5f, "Looking at health charger to use it!");

		if (bot->GetControlInterface()->IsAimOnTarget())
		{
			bot->GetControlInterface()->PressUseButton();
		}
	}

	return Continue();
}

TaskEventResponseResult<CBlackMesaBot> CBlackMesaBotFindHealthTask::OnMoveToSuccess(CBlackMesaBot* bot, CPath* path)
{
	m_timeout.StartRandom(10.0f, 15.0f);

	return TryContinue();
}

bool CBlackMesaBotFindHealthTask::IsHealthSourceValid()
{
	CBaseEntity* source = m_healthSource.Get();

	if (source == nullptr)
	{
		return false;
	}

	int effects = 0;
	entprops->GetEntProp(m_healthSource.GetEntryIndex(), Prop_Send, "m_fEffects", effects);

	if ((effects & EF_NODRAW) != 0)
	{
		return false;
	}

	if (m_isCharger)
	{
		bool on = false;
		entprops->GetEntPropBool(m_healthSource.GetEntryIndex(), Prop_Send, "m_bOn", on);

		if (!on)
		{
			return false;
		}
	}

	return true;
}