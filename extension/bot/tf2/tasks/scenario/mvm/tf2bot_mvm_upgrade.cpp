#include <limits>

#include <extension.h>
#include <bot/tf2/tf2bot.h>
#include <util/helpers.h>
#include <util/librandom.h>
#include <entities/tf2/tf_entities.h>
#include <mods/tf2/nav/tfnavarea.h>
#include <navmesh/nav_utils.h>
#include "tf2bot_mvm_upgrade.h"

#undef min // undef valve mathlib stuff that causes issues with C++ STD lib
#undef max
#undef clamp

TaskResult<CTF2Bot> CTF2BotMvMUpgradeTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	if (!SelectNearestUpgradeStation(bot))
	{
		bot->GetUpgradeManager().OnDoneUpgrading(); // Mark as done
		return Done("No upgrade stations available!");
	}

	SetGoalPosition();

	CTF2BotPathCost cost(bot);
	if (!m_nav.ComputePathToPosition(bot, m_goal, cost))
	{
		return Done("Failed to find a path to the upgrade station");
	}

	m_repathtimer.Start(2.0f);
	return Continue();
}

TaskResult<CTF2Bot> CTF2BotMvMUpgradeTask::OnTaskUpdate(CTF2Bot* bot)
{
	auto& manager = bot->GetUpgradeManager();

	if (manager.IsDoneForCurrentWave())
	{
		return Done("Done upgrading!");
	}

	if (!bot->IsInUpgradeZone())
	{
		if (m_repathtimer.IsElapsed())
		{
			m_repathtimer.Start(2.0f);

			CTF2BotPathCost cost(bot);
			if (!m_nav.ComputePathToPosition(bot, m_goal, cost))
			{
				return Done("Failed to find a path to the upgrade station");
			}
		}

		// Path to the upgrade station
		m_nav.Update(bot);
	}
	else
	{
		if (!m_buydelay.HasStarted()) // Bot reached the upgrade station, start the timer
		{
			m_buydelay.Start(randomgen->GetRandomReal<float>(1.0f, 2.0f));
		}
		else
		{
			if (m_buydelay.IsElapsed())
			{
				bot->DoMvMUpgrade();
				m_buydelay.Start(randomgen->GetRandomReal<float>(1.0f, 2.0f));
			}
		}
	}

	return Continue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotMvMUpgradeTask::OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason)
{
	m_repathtimer.Start(2.0f);

	CTF2BotPathCost cost(bot);
	if (!m_nav.ComputePathToPosition(bot, m_goal, cost))
	{
		return TryDone(PRIORITY_HIGH, "Failed to find a path to the upgrade station");
	}

	return TryContinue(PRIORITY_LOW);
}

TaskEventResponseResult<CTF2Bot> CTF2BotMvMUpgradeTask::OnMoveToSuccess(CTF2Bot* bot, CPath* path)
{
	if (!bot->IsInUpgradeZone())
	{
		return TryDone(PRIORITY_HIGH, "Reached goal but outside upgrade zone.");
	}

	return TryContinue(PRIORITY_LOW);
}

bool CTF2BotMvMUpgradeTask::SelectNearestUpgradeStation(CTF2Bot* me)
{
	std::vector<edict_t*> upgradestations;
	upgradestations.reserve(8);

	UtilHelpers::ForEachEntityOfClassname("func_upgradestation", [&upgradestations](int index, edict_t* edict, CBaseEntity* entity) {
		if (edict == nullptr)
		{
			return true; // return early, but keep looping. Upgrade stations should never have a NULL edict
		}

		tfentities::HTFBaseEntity basentity(edict);

		if (basentity.IsDisabled())
		{
			return true; // return early, keep loop
		}

		// upgrade station is not disabled, add to the list
		upgradestations.push_back(edict);

		return true; // continue looping
	});

	if (upgradestations.empty())
	{
		return false;
	}

	Vector origin = me->GetAbsOrigin();
	float best = std::numeric_limits<float>::max();
	edict_t* target = nullptr;

	for (auto upgradeentity : upgradestations)
	{
		Vector dest = UtilHelpers::getWorldSpaceCenter(upgradeentity);
		float distance = (dest - origin).Length();

		if (distance < best)
		{
			target = upgradeentity;
			best = distance;
		}

	}

	if (target == nullptr)
	{
		return false;
	}

	UtilHelpers::SetHandleEntity(m_upgradestation, target);
	return true;
}

void CTF2BotMvMUpgradeTask::SetGoalPosition()
{
	Vector center = UtilHelpers::getWorldSpaceCenter(UtilHelpers::GetEdictFromCBaseHandle(m_upgradestation));

	std::vector<CTFNavArea*> nearbyAreas;
	nearbyAreas.reserve(64);

	// collect radius was 512 but on mvm_mannworks there is only 1 upgrade station made by two brushes so this fucks things up.
	// maybe the bot should just move to a random nav area with the upgrade station attribute instead

	navutils::CollectNavAreasInRadius(center, 4096.0f, [](CTFNavArea* area) {
		if (area->HasMVMAttributes(CTFNavArea::MVMNAV_UPGRADESTATION))
		{
			return true;
		}

		return false;
	}, nearbyAreas);

	if (nearbyAreas.empty())
	{
		m_goal = center;
		return;
	}

	CTFNavArea* goalarea = nearbyAreas[randomgen->GetRandomInt<size_t>(0, nearbyAreas.size() - 1)];
	goalarea->GetClosestPointOnArea(center, &m_goal);
	m_goal.z = goalarea->GetCenter().z;
}
