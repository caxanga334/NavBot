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
	bot->SetSaxtonHale(tf2lib::vsh::IsPlayerTheSaxtonHale(bot->GetEntity()));
	m_isStomping = false;

	if (bot->IsDebugging(BOTDEBUG_TASKS) || bot->IsDebugging(BOTDEBUG_MISC))
	{
		bot->DebugPrintToConsole(255, 255, 0, "%s IS SAXTON HALE! \n", bot->GetDebugIdentifier());
	}

	m_haleAbilityCheckTimer.Start(10.0f);

	if (bot->IsSaxtonHale())
	{
		// the hale is locked at start, stop moving for a few seconds
		bot->GetMovementInterface()->StopAndWait(12.0f);
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotVSHMonitorTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (bot->IsSaxtonHale())
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
				if (!m_haleJumpTimer.IsElapsed())
				{
					// move directly towards the target when doing hale jumps
					mover->MoveTowards(threat->GetLastKnownPosition(), IMovement::MOVEWEIGHT_PRIORITY);
				}

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
						if (IsForwardJumpPossible(bot, threat))
						{
							if (bot->IsDebugging(BOTDEBUG_TASKS))
							{
								bot->DebugPrintToConsole(255, 255, 0, "%s HALE JUMP!\n", bot->GetDebugIdentifier());
							}

							input->PressJumpButton();
							m_haleJumpTimer.Start(5.0f);
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

QueryAnswerType CTF2BotVSHMonitorTask::ShouldHurry(CBaseBot* me)
{
	if (static_cast<CTF2Bot*>(me)->IsSaxtonHale())
	{
		return ANSWER_YES;
	}

	return ANSWER_UNDEFINED;
}

QueryAnswerType CTF2BotVSHMonitorTask::ShouldRetreat(CBaseBot* me)
{
	if (static_cast<CTF2Bot*>(me)->IsSaxtonHale())
	{
		return ANSWER_NO;
	}

	return ANSWER_UNDEFINED;
}

bool CTF2BotVSHMonitorTask::IsForwardJumpPossible(CTF2Bot* bot, const CKnownEntity* threat)
{
	constexpr auto MIN_RANGE_TO_JUMP = 300.0f * 300.0f;
	constexpr auto DOT_TOLERANCE = 0.90f;

	// If too close, don't bother jumping
	if (bot->GetRangeToSqr(threat->GetLastKnownPosition()) <= MIN_RANGE_TO_JUMP)
	{
		return false;
	}

	Vector start = bot->GetAbsOrigin();
	Vector to = UtilHelpers::math::BuildDirectionVector(start, threat->GetLastKnownPosition());
	Vector vel = bot->GetAbsVelocity();
	vel.NormalizeInPlace();

	float dot = DotProduct(to, vel);

	// See if the is aligned towards the target
	if (dot < DOT_TOLERANCE)
	{
		return false;
	}

	// Obstruction check
	Vector forward;
	bot->EyeVectors(&forward);
	forward.z = 0.0f;
	Vector end = start + (forward * 512.0f);
	start.z += IMovement::s_playerhull.stand_height * 3.0f;
	end.z += IMovement::s_playerhull.crouch_height;
	trace_t tr;
	trace::line(start, end, MASK_PLAYERSOLID, bot->GetEntity(), COLLISION_GROUP_PLAYER_MOVEMENT, tr);
	return tr.fraction == 1.0f;
}
