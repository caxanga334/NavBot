#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <bot/interfaces/path/chasenavigator.h>
#include "tf2bot_seek_and_destroy_jack_carrier_task.h"
#include "tf2bot_pick_jack_task.h"

CTF2BotSeekAndDestroyJackCarrierTask::CTF2BotSeekAndDestroyJackCarrierTask() :
	m_nav(CChaseNavigator::SubjectLeadType::DONT_LEAD_SUBJECT)
{
}

TaskResult<CTF2Bot> CTF2BotSeekAndDestroyJackCarrierTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	m_jack = tf2lib::passtime::GetJack();

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotSeekAndDestroyJackCarrierTask::OnTaskUpdate(CTF2Bot* bot)
{
	CBaseEntity* jack = m_jack.Get();

	if (!jack)
	{
		return Done("Jack is NULL!");
	}

	CBaseEntity* carrier = tf2lib::passtime::GetJackCarrier(jack);

	if (!carrier)
	{
		return SwitchTo(new CTF2BotPickJackTask, "Going after the jack!");
	}

	if (tf2lib::GetEntityTFTeam(carrier) == bot->GetMyTFTeam())
	{
		return Done("Teammate is carrying the jack!");
	}

	CTF2BotPathCost cost(bot);
	m_nav.Update(bot, carrier, cost);

	return Continue();
}
