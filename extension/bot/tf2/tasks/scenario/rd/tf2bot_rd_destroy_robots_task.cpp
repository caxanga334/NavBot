#include <vector>
#include <string_view>
#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <bot/interfaces/path/chasenavigator.h>
#include "tf2bot_rd_collect_points_task.h"
#include "tf2bot_rd_destroy_robots_task.h"

bool CTF2BotRDDestroyRobotsTask::IsPossible(CTF2Bot* bot, std::vector<CHandle<CBaseEntity>>& robots)
{
	using namespace std::literals::string_view_literals;
	using namespace TeamFortress2;

	constexpr auto ROBOT_CLASSNAME = "tf_robot_destruction_robot"sv;
	const TFTeam myteam = bot->GetMyTFTeam();
	auto functor = [&myteam, &robots](int index, edict_t* edict, CBaseEntity* entity) {
		if (entity)
		{
			int team = 0;
			entprops->GetEntProp(index, Prop_Send, "m_iTeamNum", team);

			// Is from the enemy team and is not invulnerable
			if (team != static_cast<int>(myteam) && !tf2lib::rd::IsRobotInvulnerable(entity))
			{
				robots.push_back(entity);
			}
		}

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname(ROBOT_CLASSNAME.data(), functor);

	return !robots.empty();
}

CTF2BotRDDestroyRobotsTask::CTF2BotRDDestroyRobotsTask(std::vector<CHandle<CBaseEntity>>&& robots) :
	m_robots(robots), m_nav(CChaseNavigator::SubjectLeadType::DONT_LEAD_SUBJECT), m_iter(0U)
{
}

TaskResult<CTF2Bot> CTF2BotRDDestroyRobotsTask::OnTaskUpdate(CTF2Bot* bot)
{
	CBaseEntity* next = nullptr;

	while (true)
	{
		if (m_iter >= m_robots.size())
		{
			std::vector<CHandle<CBaseEntity>> robots;
			
			if (!CTF2BotRDDestroyRobotsTask::IsPossible(bot, robots))
			{
				return Done("No more robots to destroy!");
			}
			else
			{
				m_robots.swap(robots);
				m_iter = 0U;
				m_nav.Invalidate();
				return Continue(); // skip one update cycle
			}
		}

		next = m_robots[m_iter].Get();

		if (!next)
		{
			m_iter++;
			continue;
		}

		break;
	}

	if (bot->GetRangeTo(next) >= 300.0f || !bot->GetSensorInterface()->IsLineOfSightClear(next))
	{
		CTF2BotPathCost cost(bot);
		m_nav.Update(bot, next, cost);
	}

	return Continue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotRDDestroyRobotsTask::OnOtherKilled(CTF2Bot* bot, CBaseEntity* pVictim, const CTakeDamageInfo& info)
{
	using namespace std::literals::string_view_literals;

	constexpr auto ROBOT_CLASSNAME = "tf_robot_destruction_robot"sv;

	if (pVictim && UtilHelpers::FClassnameIs(pVictim, ROBOT_CLASSNAME.data()))
	{
		std::vector<CHandle<CBaseEntity>> points;

		if (CTF2BotRDCollectPointsTask::IsPossible(bot, points))
		{
			m_nav.Invalidate();
			return TryPauseFor(new CTF2BotRDCollectPointsTask(std::move(points)), PRIORITY_HIGH, "Collecting points from destroyed robot!");
		}
	}

	return TryContinue();
}
