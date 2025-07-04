#include NAVBOT_PCH_FILE
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

bool CTF2BotRDCollectPointsTask::IsPossible(CTF2Bot* bot, std::vector<CHandle<CBaseEntity>>& points)
{
	using namespace std::literals::string_view_literals;

	constexpr auto POINT_ENTITY_CLASSNAME = "item_bonuspack"sv;

	UtilHelpers::CEntityEnumerator enumerator;
	UtilHelpers::EntitiesInSphere(bot->GetAbsOrigin(), 2048.0f, enumerator);

	auto myteam = bot->GetMyTFTeam();
	auto functor = [&POINT_ENTITY_CLASSNAME, &points, &myteam](CBaseEntity* entity) -> bool {
		if (tf2lib::GetEntityTFTeam(entity) == myteam && UtilHelpers::FClassnameIs(entity, POINT_ENTITY_CLASSNAME.data()))
		{
			points.push_back(entity);
		}

		return true;
	};
	enumerator.ForEach(functor);

	return !points.empty();
}

CTF2BotRDCollectPointsTask::CTF2BotRDCollectPointsTask(std::vector<CHandle<CBaseEntity>>&& points) :
	m_points(points), m_nav(CChaseNavigator::SubjectLeadType::DONT_LEAD_SUBJECT), m_iter(0U)
{
}

TaskResult<CTF2Bot> CTF2BotRDCollectPointsTask::OnTaskUpdate(CTF2Bot* bot)
{
	CBaseEntity* next = nullptr;

	while (true)
	{
		if (m_iter >= m_points.size())
		{
			return Done("No more points to collect!");
		}

		next = m_points[m_iter].Get();

		if (!next)
		{
			m_iter++;
			continue;
		}

		break;
	}

	CTF2BotPathCost cost(bot);
	m_nav.Update(bot, next, cost);

	return Continue();
}
