#include <vector>
#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <util/librandom.h>
#include <bot/blackmesa/bmbot.h>
#include "bmbot_find_health_task.h"

bool CBlackMesaBotFindHealthTask::IsPossible(CBlackMesaBot* bot)
{
	if (bot->GetHealthPercentage() > 0.70f)
	{
		return false;
	}

	return true;
}

TaskResult<CBlackMesaBot> CBlackMesaBotFindHealthTask::OnTaskStart(CBlackMesaBot* bot, AITask<CBlackMesaBot>* pastTask)
{
	CBaseEntity* healthSource = nullptr;

	if (!CBlackMesaBotFindHealthTask::FindHealthSource(bot, &healthSource))
	{
		return Done("No health source nearby!");
	}

	SetHealthSource(healthSource, bot);

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
		CBaseEntity* newSource = nullptr;

		if (!CBlackMesaBotFindHealthTask::FindHealthSource(bot, &newSource, 1024.0f, true))
		{
			return Done("No more health sources to use!");
		}

		SetHealthSource(newSource, bot);
		m_repathtimer.Invalidate();
	}

	float moveToRange = m_isCharger ? 70.0f : 24.0f;

	if (bot->GetRangeTo(m_goal) > moveToRange)
	{
		if (m_repathtimer.IsElapsed())
		{
			m_repathtimer.Start(1.0f);
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

void CBlackMesaBotFindHealthTask::SetHealthSource(CBaseEntity* source, CBlackMesaBot* bot)
{
	m_healthSource = source;
	m_goal = UtilHelpers::getWorldSpaceCenter(source);
	float range = bot->GetRangeTo(m_goal);
	float time = range / 90.0f;
	m_isCharger = UtilHelpers::FClassnameIs(source, "item_healthcharger");
	m_timeout.Start(time + 10.0f);
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

bool CBlackMesaBotFindHealthTask::FindHealthSource(CBlackMesaBot* bot, CBaseEntity** healthSource, const float maxRange, const bool filterByDistance)
{
	// vector of health sources
	std::vector<CBaseEntity*> sources;
	Vector start = bot->GetAbsOrigin();

	auto chargerfunc = [&sources, &start, &maxRange](int index, edict_t* edict, CBaseEntity* entity) {
		if (entity)
		{
			float charge = -1.0f;
			entprops->GetEntPropFloat(index, Prop_Send, "m_flCharge", charge);

			const Vector& end = UtilHelpers::getEntityOrigin(entity);
			float range = (start - end).Length();

			if (charge > 0.01f && range <= maxRange)
			{
				sources.push_back(entity);
			}
		}

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("item_healthcharger", chargerfunc);

	auto kitfunc = [&sources, &start, &maxRange](int index, edict_t* edict, CBaseEntity* entity) {
		if (entity)
		{
			const Vector& end = UtilHelpers::getEntityOrigin(entity);
			float range = (start - end).Length();
			int effects = 0;
			entprops->GetEntProp(index, Prop_Send, "m_fEffects", effects);

			if ((effects & EF_NODRAW) != 0)
			{
				return true; // continue
			}

			if (range <= maxRange)
			{
				sources.push_back(entity);
			}
		}

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("item_healthkit", kitfunc);

	if (sources.empty())
	{
		return false;
	}

	if (sources.size() == 1)
	{
		*healthSource = sources[0];
		return true;
	}

	// select the nearest health source
	if (filterByDistance)
	{
		float best = maxRange;
		CBaseEntity* out = nullptr;

		for (CBaseEntity* entity : sources)
		{
			const Vector& end = UtilHelpers::getEntityOrigin(entity);
			float range = (start - end).Length();

			if (range < best)
			{
				best = range;
				out = entity;
			}
		}

		if (out == nullptr)
		{
			// this shouldn't happen
			return false;
		}

		*healthSource = out;
	}
	else
	{
		// select a random health source
		*healthSource = librandom::utils::GetRandomElementFromVector<CBaseEntity*>(sources);
	}

	return true;
}
