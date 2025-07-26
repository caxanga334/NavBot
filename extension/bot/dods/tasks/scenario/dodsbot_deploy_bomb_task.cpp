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
#include "dodsbot_deploy_bomb_task.h"
#include "dodsbot_fetch_bomb_task.h"

CDoDSBotDeployBombTask::CDoDSBotDeployBombTask(CBaseEntity* target)
{
	m_target = target;
}

TaskResult<CDoDSBot> CDoDSBotDeployBombTask::OnTaskStart(CDoDSBot* bot, AITask<CDoDSBot>* pastTask)
{
	if (bot->IsDebugging(BOTDEBUG_TASKS))
	{
		META_CONPRINTF("%s BOMBING TARGET %i [%p]\n", bot->GetDebugIdentifier(), m_target.GetEntryIndex(), m_target.Get());
	}

	return Continue();
}

TaskResult<CDoDSBot> CDoDSBotDeployBombTask::OnTaskUpdate(CDoDSBot* bot)
{
	CBaseEntity* target = m_target.Get();

	if (!target)
	{
		return Done("Bomb target is NULL!");
	}

	int state = 0;
	entprops->GetEntProp(m_target.GetEntryIndex(), Prop_Send, "m_iState", state);

	if (state == static_cast<int>(dayofdefeatsource::DoDBombTargetState::BOMB_TARGET_ARMED))
	{
		return Done("Bomb has been planted!");
	}

	if (state != static_cast<int>(dayofdefeatsource::DoDBombTargetState::BOMB_TARGET_ACTIVE))
	{
		return Done("Bomb target is invalid!");
	}

	if (!bot->GetInventoryInterface()->HasBomb())
	{
		return PauseFor(new CDoDSBotFetchBombTask(nullptr), "I don't have a bomb, going to get one!");
	}

	Vector eyePos = bot->GetEyeOrigin();
	Vector center = UtilHelpers::getEntityOrigin(target);

	/*

	auto extraHitFunc = [&target, &bot](IHandleEntity* pHandleEntity, int contentsMask) {

		CBaseEntity* pEntity = trace::EntityFromEntityHandle(pHandleEntity);

		if (pEntity == target || pEntity == bot->GetEntity())
		{
			return false; // don't hit the target or the bot
		}

		if (pEntity)
		{
			if (UtilHelpers::IsPlayer(pEntity))
			{
				return false; // don't hit players
			}
		}

		// hit everything else
		return true;
	};

	
	trace::CTraceFilterSimple filter(bot->GetEntity(), COLLISION_GROUP_NONE, extraHitFunc);
	trace_t tr;
	// https://github.com/ValveSoftware/source-sdk-2013/blob/0565403b153dfcde602f6f58d8f4d13483696a13/src/game/shared/baseplayer_shared.cpp#L1078
	constexpr unsigned int useableContents = MASK_SOLID | CONTENTS_DEBRIS | CONTENTS_PLAYERCLIP;
	trace::line(eyePos, center, useableContents, &filter, tr);
	*/

	// DoD has special checks for bomb target that doesn't use trace lines
	// https://github.com/lua9520/source-engine-2018-hl2_src/blob/3bf9df6b2785fa6d951086978a3e66f49427166a/game/server/dod/dod_player.cpp#L4107-L4143

	// In USE range of the bomb target
	if ((eyePos - center).Length() < CBaseExtPlayer::PLAYER_USE_RADIUS /* && tr.fraction == 1.0f */)
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
