#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <sdkports/sdk_ehandle.h>
#include <mods/tf2/tf2lib.h>
#include <bot/tf2/tf2bot.h>
#include <bot/interfaces/path/meshnavigator.h>
#include "tf2bot_pd_move_to_capture_zone_task.h"

CTF2BotPDMoveToCaptureZoneTask::CTF2BotPDMoveToCaptureZoneTask(CBaseEntity* captureZone) :
	m_capzone(captureZone)
{
	m_pathfailures = 0;
}

bool CTF2BotPDMoveToCaptureZoneTask::IsPossible(CTF2Bot* bot, CBaseEntity** captureZone)
{
	// no points to deliver
	if (bot->GetItem() == nullptr)
	{
		return false;
	}

	CBaseEntity* pEntity = nullptr;

	UtilHelpers::ForEachEntityOfClassname("func_capturezone", [&bot, &pEntity](int index, edict_t* edict, CBaseEntity* entity) {
		if (entity && tf2lib::GetEntityTFTeam(index) == bot->GetMyTFTeam())
		{
			bool disabled = false;
			entprops->GetEntPropBool(index, Prop_Data, "m_bDisabled", disabled);

			if (disabled)
			{
				return true;
			}

			pEntity = entity;
			return false; // stop on the first found, maps generally only have one
		}

		return true;
	});

	if (!pEntity)
	{
		return false; // no enabled capture zone found
	}

	CTF2BotPathCost cost(bot);
	CMeshNavigator nav;
	Vector pos = UtilHelpers::getWorldSpaceCenter(pEntity);

	// On some maps, the capture zone is always enabled, consider possible if it's reachable
	if (nav.ComputePathToPosition(bot, pos, cost))
	{
		*captureZone = pEntity;
		return true;
	}

	return false;
}

TaskResult<CTF2Bot> CTF2BotPDMoveToCaptureZoneTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	// the capture zone is available for a limited time, until a way to get the exact time is found, just use a random time
	m_timeout.StartRandom(40.0f, 50.0f);

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotPDMoveToCaptureZoneTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (m_timeout.IsElapsed())
	{
		return Done("Task timed out!");
	}

	CBaseEntity* zone = m_capzone.Get();

	if (!zone)
	{
		return Done("Capture zone entity is NULL!");
	}

	bool disabled = false;
	entprops->GetEntPropBool(m_capzone.GetEntryIndex(), Prop_Data, "m_bDisabled", disabled);

	if (disabled)
	{
		return Done("Capture zone is disabled!");
	}

	edict_t* flag = bot->GetItem();

	if (!flag)
	{
		return Done("No more points to deliver!");
	}

	if (m_repathtimer.IsElapsed())
	{
		// on some maps, the zone is attached to a moving entity, needs faster repaths
		m_repathtimer.Start(1.0f);
		Vector pos = UtilHelpers::getWorldSpaceCenter(zone);
		CTF2BotPathCost cost(bot);

		if (!m_nav.ComputePathToPosition(bot, pos, cost, 0.0f, true))
		{
			if (++m_pathfailures > 20)
			{
				return Done("Giving up, too many path failures!");
			}
		}
	}

	m_nav.Update(bot);

	return Continue();
}

TaskEventResponseResult<CTF2Bot> CTF2BotPDMoveToCaptureZoneTask::OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason)
{
	++m_pathfailures;
	return TryContinue();
}

QueryAnswerType CTF2BotPDMoveToCaptureZoneTask::ShouldSeekAndDestroy(CBaseBot* me, const CKnownEntity* them)
{
	return ANSWER_NO;
}

QueryAnswerType CTF2BotPDMoveToCaptureZoneTask::ShouldHurry(CBaseBot* me)
{
	return ANSWER_YES;
}

QueryAnswerType CTF2BotPDMoveToCaptureZoneTask::ShouldRetreat(CBaseBot* me)
{
	return ANSWER_NO;
}

QueryAnswerType CTF2BotPDMoveToCaptureZoneTask::ShouldAssistTeammate(CBaseBot* me, CBaseEntity* teammate)
{
	return ANSWER_NO;
}
