#include NAVBOT_PCH_FILE
#include <mods/css/nav/css_nav_mesh.h>
#include <mods/css/css_lib.h>
#include <mods/css/css_mod.h>
#include <bot/css/cssbot.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <bot/tasks_shared/bot_shared_take_cover_from_spot.h>
#include "cssbot_defuse_bomb_task.h"

TaskResult<CCSSBot> CCSSBotDefuseBombTask::OnTaskStart(CCSSBot* bot, AITask<CCSSBot>* pastTask)
{
	m_hasdefuser = csslib::HasDefuser(bot->GetEntity());
	m_defusing = false;
	return Continue();
}

TaskResult<CCSSBot> CCSSBotDefuseBombTask::OnTaskUpdate(CCSSBot* bot)
{
	CCounterStrikeSourceMod* csmod = CCounterStrikeSourceMod::GetCSSMod();

	if (!csmod->IsBombActive())
	{
		return Done("Bomb was defused or exploded.");
	}

	CBaseEntity* c4 = csmod->GetActiveBombEntity();

	if (!c4)
	{
		return Done("Bomb entity is NULL!");
	}

	const float defusetime = m_hasdefuser ? 5.0f : 10.0f;
	const float timetoexplode = csslib::GetC4TimeRemaining(c4);
	const Vector& pos = UtilHelpers::getEntityOrigin(c4);

	// no longer possible to defuse it
	if (timetoexplode < defusetime)
	{
		if (!m_defusing)
		{
			const CBotWeapon* knife = bot->GetInventoryInterface()->FindWeaponBySlot(counterstrikesource::CSS_WEAPON_SLOT_KNIFE);

			if (knife)
			{
				bot->GetInventoryInterface()->EquipWeapon(knife);
			}

			return SwitchTo(new CBotSharedTakeCoverFromSpotTask<CCSSBot, CCSSBotPathCost>(bot, pos, 2048.0f, false, false, 10000.0f), "Running away from exploding bomb!");
		}
	}

	const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(ISensor::ANY_THREATS);

	if (threat)
	{
		if (threat->IsVisibleNow())
		{
			// stop defusing, scenario behavior will handle attacking for us
			if (m_defusing)
			{
				m_defusing = false;
				bot->GetControlInterface()->StopAiming(IPlayerController::LOOK_CRITICAL);
			}

			return Continue();
		}
	}

	const float dist = (bot->GetEyeOrigin() - pos).Length();

	if (dist < CBaseExtPlayer::PLAYER_USE_RADIUS && bot->GetSensorInterface()->IsLineOfSightClear(pos))
	{
		bot->GetControlInterface()->AimAt(pos, IPlayerController::LOOK_CRITICAL, 1.0f, "Looking at bomb to disarm it!");
		
		if (bot->GetControlInterface()->IsAimOnTarget())
		{
			bot->GetControlInterface()->PressUseButton();
		}

		m_defusing = csslib::IsDefusing(bot->GetEntity());
	}
	else
	{
		if (!m_nav.IsValid() || m_nav.NeedsRepath())
		{
			CCSSBotPathCost cost{ bot, RouteType::FASTEST_ROUTE };
			m_nav.ComputePathToPosition(bot, pos, cost);
			m_nav.StartRepathTimer();
		}

		m_nav.Update(bot);
	}

	return Continue();
}

QueryAnswerType CCSSBotDefuseBombTask::ShouldAttack(CBaseBot* me, const CKnownEntity* them)
{
	if (m_defusing)
	{
		return ANSWER_NO;
	}

	return ANSWER_UNDEFINED;
}

QueryAnswerType CCSSBotDefuseBombTask::ShouldSeekAndDestroy(CBaseBot* me, const CKnownEntity* them)
{
	if (m_defusing)
	{
		return ANSWER_NO;
	}

	return ANSWER_UNDEFINED;
}

QueryAnswerType CCSSBotDefuseBombTask::ShouldRetreat(CBaseBot* me)
{
	if (m_defusing)
	{
		return ANSWER_NO;
	}

	return ANSWER_UNDEFINED;
}

QueryAnswerType CCSSBotDefuseBombTask::ShouldAssistTeammate(CBaseBot* me, CBaseEntity* teammate)
{
	if (m_defusing)
	{
		return ANSWER_NO;
	}

	return ANSWER_UNDEFINED;
}

QueryAnswerType CCSSBotDefuseBombTask::ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon)
{
	if (m_defusing)
	{
		return ANSWER_NO;
	}

	return ANSWER_UNDEFINED;
}
