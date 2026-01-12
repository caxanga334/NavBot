#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <sdkports/sdk_traces.h>
#include <sdkports/sdk_ehandle.h>
#include <sdkports/sdk_timers.h>
#include <mods/dods/dayofdefeatsourcemod.h>
#include <bot/dods/dodsbot.h>
#include <bot/interfaces/path/meshnavigator.h>
#include "dodsbot_defuse_bomb_task.h"

CDoDSBotDefuseBombTask::CDoDSBotDefuseBombTask(CBaseEntity* target) :
	m_target(target)
{
}

CDoDSBotDefuseBombTask::CDoDSBotDefuseBombTask(CBaseEntity* target, CDoDSBot* bot) :
	m_target(target), m_defuseNotification(bot, m_target.GetEntryIndex())
{
}

TaskResult<CDoDSBot> CDoDSBotDefuseBombTask::OnTaskStart(CDoDSBot* bot, AITask<CDoDSBot>* pastTask)
{
	CBaseEntity* target = m_target.Get();

	if (!target)
	{
		return Done("Bomb target is NULL!");
	}

	m_defuseNotification.Notify(bot, m_target.GetEntryIndex());
	return Continue();
}

TaskResult<CDoDSBot> CDoDSBotDefuseBombTask::OnTaskUpdate(CDoDSBot* bot)
{
	CBaseEntity* target = m_target.Get();

	if (!target)
	{
		return Done("Bomb target is NULL!");
	}

	int state = 0;
	entprops->GetEntProp(m_target.GetEntryIndex(), Prop_Send, "m_iState", state);

	if (state != static_cast<int>(dayofdefeatsource::DoDBombTargetState::BOMB_TARGET_ARMED))
	{
		return Done("Bomb has been defused or exploded!");
	}

	Vector eyePos = bot->GetEyeOrigin();
	Vector center = UtilHelpers::getEntityOrigin(target);

	// In USE range of the bomb target
	if ((eyePos - center).Length() < CBaseExtPlayer::PLAYER_USE_RADIUS)
	{
		bot->GetControlInterface()->AimAt(center, IPlayerController::LOOK_CRITICAL, 0.5f, "Looking at bomb target to plant bomb!");

		if (bot->IsLookingTowards(center, 0.84f))
		{
			bot->GetControlInterface()->PressUseButton();
		}
	}
	else
	{
		if (m_nav.NeedsRepath())
		{
			m_nav.StartRepathTimer();
			CDoDSBotPathCost cost(bot);
			m_nav.ComputePathToPosition(bot, center, cost, 0.0f, true);
		}

		m_nav.Update(bot);
	}

	return Continue();
}

TaskEventResponseResult<CDoDSBot> CDoDSBotDefuseBombTask::OnBombDefused(CDoDSBot* bot, const Vector& position, const int teamIndex, CBaseEntity* player, CBaseEntity* ent)
{
	// TO-DO
	return TryContinue(PRIORITY_LOW);
}

bool CDoDSBotDefuseBombTask::IsBehaviorRunning(int id, int flags, bool ismod)
{
	if (ismod && id == IDENTIFIER)
	{
		return true;
	}

	return false;
}
