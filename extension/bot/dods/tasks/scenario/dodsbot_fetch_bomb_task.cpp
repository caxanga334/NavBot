#include NAVBOT_PCH_FILE
#include <extension.h>
#include <mods/dods/nav/dods_nav_mesh.h>
#include <mods/dods/nav/dods_nav_area.h>
#include <bot/bot_shared_utils.h>
#include <mods/dods/dayofdefeatsourcemod.h>
#include <bot/dods/dodsbot.h>
#include <bot/interfaces/path/meshnavigator.h>
#include "dodsbot_fetch_bomb_task.h"

class CDoDSBombDispenserFilter : public UtilHelpers::IGenericFilter<CBaseEntity*>
{
public:
	CDoDSBombDispenserFilter(CDoDSBot* dodbot)
	{
		this->bot = dodbot;
		this->dispenser_area = nullptr;
	}

	bool IsSelected(CBaseEntity* object) override
	{
		const int teamIndex = static_cast<int>(this->bot->GetMyDoDTeam());
		Vector pos = UtilHelpers::getWorldSpaceCenter(object);
		this->dispenser_area = static_cast<CDoDSNavArea*>(TheNavMesh->GetNearestNavArea(pos, CPath::PATH_GOAL_MAX_DISTANCE_TO_AREA * 2.0f, false, true, teamIndex));

		// entity is off the nav-mesh (unreachable)
		if (!this->dispenser_area)
		{
			return false;
		}

		int dispenseTeam = 0;
		entprops->GetEntProp(object, Prop_Data, "m_iDispenseToTeam", dispenseTeam);

		// Team specific dispenser
		if (dispenseTeam != 0 && dispenseTeam != teamIndex)
		{
			return false;
		}

		bool disabled = false;
		entprops->GetEntPropBool(object, Prop_Data, "m_bDisabled", disabled);

		// entity is disabled
		if (disabled)
		{
			return false;
		}

		return true;
	}

	CDoDSBot* bot;
	CDoDSNavArea* dispenser_area;
};

CDoDSBotFetchBombTask::CDoDSBotFetchBombTask(AITask<CDoDSBot>* exitTask, CBaseEntity* dispenser) :
	m_goal(0.0f, 0.0f, 0.0f), m_dispenser(dispenser)
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

bool CDoDSBotFetchBombTask::IsPossible(CDoDSBot* bot, CBaseEntity** dispenser)
{
	botsharedutils::IsReachableAreas collector{ bot, 12000.0f };
	collector.Execute();
	CDoDSBombDispenserFilter filter{ bot };

	if (collector.IsCollectedAreasEmpty())
	{
		return false;
	}

	float distance = std::numeric_limits<float>::max();
	auto func = [&dispenser, &distance, &filter, &collector](int index, edict_t* edict, CBaseEntity* entity) {
		if (entity)
		{
			if (filter.IsSelected(entity))
			{
				float cost = 0.0f;

				if (collector.IsReachable(filter.dispenser_area, &cost))
				{
					if (cost < distance)
					{
						distance = cost;
						*dispenser = entity;
					}
				}
			}
		}

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("dod_bomb_dispenser", func);

	return *dispenser != nullptr;
}

TaskResult<CDoDSBot> CDoDSBotFetchBombTask::OnTaskStart(CDoDSBot* bot, AITask<CDoDSBot>* pastTask)
{
	CBaseEntity* dispenser = m_dispenser.Get();

	if (!dispenser)
	{
		if (!CDoDSBotFetchBombTask::IsPossible(bot, &dispenser))
		{
			return Done("No bomb dispenser to fetch bomb from!");
		}
	}

	m_goal = UtilHelpers::getWorldSpaceCenter(dispenser);
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

	if (m_nav.NeedsRepath())
	{
		m_nav.StartRepathTimer();
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
