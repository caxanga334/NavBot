#include NAVBOT_PCH_FILE
#include <extension.h>
#include <mods/tf2/tf2lib.h>
#include <mods/tf2/teamfortress2mod.h>
#include <bot/tf2/tf2bot.h>
#include <bot/tasks_shared/bot_shared_simple_deathmatch_behavior.h>
#include "tf2bot_vsh_monitor_task.h"

AITask<CTF2Bot>* CTF2BotVSHMonitorTask::InitialNextTask(CTF2Bot* bot)
{
	return new CBotSharedSimpleDMBehaviorTask<CTF2Bot, CTF2BotPathCost>();
}

TaskResult<CTF2Bot> CTF2BotVSHMonitorTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	m_isHale = tf2lib::vsh::IsPlayerTheSaxtonHale(bot->GetEntity());
	m_isStomping = false;

	if (bot->IsDebugging(BOTDEBUG_TASKS) && bot->IsDebugging(BOTDEBUG_MISC))
	{
		bot->DebugPrintToConsole(255, 255, 0, "%s IS SAXTON HALE! \n", bot->GetDebugIdentifier());
	}

	m_haleAbilityCheckTimer.Start(10.0f);

	if (m_isHale)
	{
		// the hale is locked at start, stop moving for a few seconds
		bot->GetMovementInterface()->StopAndWait(7.0f);
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotVSHMonitorTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (m_isHale)
	{
		auto input = bot->GetControlInterface();
		auto mover = bot->GetMovementInterface();
		const bool isOnGround = mover->IsOnGround();

		if (m_isStomping && isOnGround)
		{
			m_isStomping = false;
			input->ReleaseCrouchButton();
		}

		if (!m_chargeTimer.IsElapsed())
		{
			input->PressReloadButton(0.2f);
		}

		if (m_haleAbilityCheckTimer.IsElapsed())
		{
			m_haleAbilityCheckTimer.Start(0.5f);

			const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(ISensor::ONLY_VISIBLE_THREATS);

			if (threat)
			{
				if (m_stompCooldownTimer.IsElapsed() && m_chargeTimer.IsElapsed())
				{
					if (!isOnGround)
					{
						// airborne, stomp check

						if ((bot->GetAbsOrigin() - threat->GetLastKnownPosition()).AsVector2D().IsLengthLessThan(400.0f))
						{
							m_stompCooldownTimer.Start(20.0f);
							bot->GetControlInterface()->PressCrouchButton(10.0f);
							m_isStomping = true;
						}
						else
						{
							// too far, try jumping
							
							input->AimAt(threat->GetEntity(), IPlayerController::LOOK_COMBAT, 1.0f, "Looking at threat!");

							if (input->IsAimOnTarget())
							{
								// use hale super jump to try to get closer
								mover->MoveTowards(threat->GetLastKnownPosition(), IMovement::MOVEWEIGHT_PRIORITY);
								input->PressJumpButton();
							}
						}
					}
					else
					{
						if (IsForwardJumpPossible(bot))
						{
							input->PressJumpButton();
						}
					}
				}
				
				if (m_chargeCooldownTimer.IsElapsed())
				{
					m_chargeTimer.Start(4.0f);
					m_chargeCooldownTimer.StartRandom(20.0f, 30.0f);
				}
			}
		}
	}

	return Continue();
}

bool CTF2BotVSHMonitorTask::IsForwardJumpPossible(CTF2Bot* bot)
{
	Vector start = bot->GetAbsOrigin();
	Vector forward;
	bot->EyeVectors(&forward);
	forward.z = 0.0f;
	Vector end = start + (forward * 512.0f);
	start.z += CTF2BotMovement::PLAYER_HULL_STAND * 5.0f;
	trace_t tr;
	trace::line(start, end, MASK_PLAYERSOLID, bot->GetEntity(), COLLISION_GROUP_PLAYER_MOVEMENT, tr);
	return tr.fraction == 1.0f;
}
