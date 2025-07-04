#include NAVBOT_PCH_FILE
#include <queue>
#include <vector>
#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <sdkports/sdk_ehandle.h>
#include <sdkports/sdk_timers.h>
#include <mods/dods/dayofdefeatsourcemod.h>
#include <bot/dods/dodsbot.h>
#include <bot/interfaces/path/meshnavigator.h>
#include "dodsbot_fetch_bomb_task.h"

CDoDSBotFetchBombTask::CDoDSBotFetchBombTask(AITask<CDoDSBot>* exitTask) :
	m_goal(0.0f, 0.0f, 0.0f)
{
	m_exittask = exitTask;
}

CDoDSBotFetchBombTask::~CDoDSBotFetchBombTask()
{
	if (m_exittask)
	{
		delete m_exittask;
	}
}

TaskResult<CDoDSBot> CDoDSBotFetchBombTask::OnTaskStart(CDoDSBot* bot, AITask<CDoDSBot>* pastTask)
{
	if (!SelectNearestBombDispenser(bot))
	{
		return Done("No bomb dispenser to fetch bomb from!");
	}

	return Continue();
}

TaskResult<CDoDSBot> CDoDSBotFetchBombTask::OnTaskUpdate(CDoDSBot* bot)
{
	if (bot->GetInventoryInterface()->HasBomb())
	{
		if (m_exittask)
		{
			AITask<CDoDSBot>* next = m_exittask;
			m_exittask = nullptr; // set to NULL so our destructor doesn't delete it
			return SwitchTo(next, "I have a bomb!");
		}
		else
		{
			return Done("I have a bomb!");
		}
	}

	if (m_repathtimer.IsElapsed())
	{
		m_repathtimer.Start(CBaseBot::s_botrng.GetRandomReal<float>(1.0f, 2.1f));
		CDoDSBotPathCost cost(bot);
		m_nav.ComputePathToPosition(bot, m_goal, cost, 0.0f, true);
	}

	m_nav.Update(bot);

	return Continue();
}

TaskEventResponseResult<CDoDSBot> CDoDSBotFetchBombTask::OnMoveToSuccess(CDoDSBot* bot, CPath* path)
{
	if (bot->GetInventoryInterface()->HasBomb())
	{
		if (m_exittask)
		{
			AITask<CDoDSBot>* next = m_exittask;
			m_exittask = nullptr; // set to NULL so our destructor doesn't delete it
			return TrySwitchTo(next, PRIORITY_LOW, "I have a bomb!");
		}
		else
		{
			return TryDone(PRIORITY_LOW, "I have a bomb!");
		}
	}

	return TryContinue();
}

bool CDoDSBotFetchBombTask::SelectNearestBombDispenser(CDoDSBot* bot)
{
	auto cmp = [&bot](CBaseEntity* lhs, CBaseEntity* rhs) {
		float range1 = bot->GetRangeToSqr(UtilHelpers::getWorldSpaceCenter(lhs));
		float range2 = bot->GetRangeToSqr(UtilHelpers::getWorldSpaceCenter(rhs));

		return range1 > range2;
	};

	std::priority_queue<CBaseEntity*, std::vector<CBaseEntity*>, decltype(cmp)> nearest_dispenser_queue(cmp);

	auto functor = [&nearest_dispenser_queue, &bot](int index, edict_t* edict, CBaseEntity* entity) {

		if (entity)
		{
			bool disabled = false;
			entprops->GetEntPropBool(index, Prop_Data, "m_bDisabled", disabled);

			if (disabled)
			{
				return true; // exit early, keep loop
			}

			int teamIndex = static_cast<int>(dayofdefeatsource::DoDTeam::DODTEAM_UNASSIGNED);
			entprops->GetEntProp(index, Prop_Data, "m_iDispenseToTeam", teamIndex);

			if (teamIndex > static_cast<int>(dayofdefeatsource::DoDTeam::DODTEAM_SPECTATOR) && teamIndex != static_cast<int>(bot->GetMyDoDTeam()))
			{
				return true; // has team restriction and not for my team
			}

			nearest_dispenser_queue.push(entity);
		}

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("dod_bomb_dispenser", functor);

	if (nearest_dispenser_queue.empty())
	{
		return false;
	}

	m_goal = UtilHelpers::getWorldSpaceCenter(nearest_dispenser_queue.top());
	return true;
}
