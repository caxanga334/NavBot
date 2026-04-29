#include NAVBOT_PCH_FILE
#include <mods/css/nav/css_nav_mesh.h>
#include <mods/css/css_lib.h>
#include <bot/css/cssbot.h>
#include <bot/interfaces/path/meshnavigator.h>
#include "cssbot_plant_bomb_task.h"

bool CCSSBotPlantBombTask::IsPossible(CCSSBot* bot, Vector& plantspot)
{
	if (bot->GetMyCSSTeam() != counterstrikesource::CSSTeam::TERRORIST)
	{
		return false;
	}

	if (!csslib::MapHasBombTargets())
	{
		return false;
	}

	if (!bot->GetInventoryInterface()->IsBombCarrier())
	{
		return false;
	}

	CBaseEntity* bombsite = UtilHelpers::GetRandomEntityOfClassname("func_bomb_target");

	if (!bombsite)
	{
		return false;
	}

	std::vector<CNavArea*> areas;
	TheCSSNavMesh()->CollectAreasTouchingEntity(bombsite, true, areas);

	if (areas.empty())
	{
		if (bot->IsDebugging(BOTDEBUG_TASKS))
		{
			Vector center = UtilHelpers::getWorldSpaceCenter(bombsite);
			bot->DebugPrintToConsole(255, 0, 0, "%s BOMB SITE \"%s\" %s DOESN'T CONTAIN ANY NAV AREAS! \n", 
				bot->GetDebugIdentifier(), UtilHelpers::textformat::FormatEntity(bombsite), UtilHelpers::textformat::FormatVector(center));
		}

		return false;
	}

	CNavArea* area = librandom::utils::GetRandomElementFromVector(areas);
	plantspot = area->GetCenter();
	return true;
}

CCSSBotPlantBombTask::CCSSBotPlantBombTask(const Vector& plantspot)
{
	m_moveGoal = plantspot;
	m_isplanting = false;
}

TaskResult<CCSSBot> CCSSBotPlantBombTask::OnTaskUpdate(CCSSBot* bot)
{
	if (!bot->GetInventoryInterface()->IsBombCarrier())
	{
		return Done("No longer carrying a bomb!");
	}

	if (m_timeout.HasStarted() && m_timeout.IsElapsed())
	{
		return Done("Timeout timer is elapsed!");
	}

	if (!m_isplanting && csslib::IsInBombPlantZone(bot->GetEntity()))
	{
		m_isplanting = true;
		m_timeout.Start(15.0f);
		bot->GetControlInterface()->ReleaseAllAttackButtons();
		bot->GetMovementInterface()->DoCounterStrafe(1.0f);
	}

	if (m_isplanting)
	{
		bot->GetCombatInterface()->DisableCombat(0.2f);

		const CBotWeapon* activeWeapon = bot->GetInventoryInterface()->GetActiveBotWeapon();

		if (!activeWeapon || !activeWeapon->IsWeapon("weapon_c4"))
		{
			const CBotWeapon* c4 = bot->GetInventoryInterface()->FindWeaponByClassname("weapon_c4");

			if (c4)
			{
				bot->GetInventoryInterface()->EquipWeapon(c4);
			}

			return Continue();
		}

		bot->GetControlInterface()->PressCrouchButton(0.5f);
		bot->GetControlInterface()->PressAttackButton(0.5f);
		Vector vLook = bot->GetAbsOrigin();
		vLook.z -= 128.0f;
		bot->GetControlInterface()->AimAt(vLook, IPlayerController::LOOK_CRITICAL, 1.0f, "Looking at ground to plant the c4!");
	}
	else
	{
		if (!m_nav.IsValid() || m_nav.NeedsRepath())
		{
			CCSSBotPathCost cost{ bot, RouteType::SAFEST_ROUTE };
			m_nav.ComputePathToPosition(bot, m_moveGoal, cost);
			m_nav.StartRepathTimer();
		}

		m_nav.Update(bot);
	}

	return Continue();
}

void CCSSBotPlantBombTask::OnTaskEnd(CCSSBot* bot, AITask<CCSSBot>* nextTask)
{
	bot->GetCombatInterface()->EnableCombat();
}

QueryAnswerType CCSSBotPlantBombTask::ShouldAttack(CBaseBot* me, const CKnownEntity* them)
{
	if (m_isplanting)
	{
		return ANSWER_NO;
	}

	return ANSWER_UNDEFINED;
}

QueryAnswerType CCSSBotPlantBombTask::ShouldSeekAndDestroy(CBaseBot* me, const CKnownEntity* them)
{
	if (m_isplanting)
	{
		return ANSWER_NO;
	}

	return ANSWER_UNDEFINED;
}

QueryAnswerType CCSSBotPlantBombTask::ShouldRetreat(CBaseBot* me)
{
	if (m_isplanting)
	{
		return ANSWER_NO;
	}

	return ANSWER_UNDEFINED;
}

QueryAnswerType CCSSBotPlantBombTask::ShouldAssistTeammate(CBaseBot* me, CBaseEntity* teammate)
{
	return ANSWER_NO;
}

QueryAnswerType CCSSBotPlantBombTask::ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon)
{
	if (m_isplanting)
	{
		return ANSWER_NO;
	}

	return ANSWER_UNDEFINED;
}
