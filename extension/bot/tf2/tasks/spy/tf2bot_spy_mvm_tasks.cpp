#include <cstring>
#include <vector>
#include <extension.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/nav/tfnavmesh.h>
#include <mods/tf2/nav/tfnavarea.h>
#include <navmesh/nav_pathfind.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <bot/bot_shared_utils.h>
#include <util/entprops.h>
#include <util/helpers.h>
#include <entities/tf2/tf_entities.h>
#include <bot/interfaces/path/chasenavigator.h>
#include "tf2bot_spy_mvm_tasks.h"

#if SOURCE_ENGINE == SE_TF2
static ConVar cvar_allow_sapping_robots("sm_navbot_tf_spy_can_sap_robots", "0", FCVAR_GAMEDLL, "If enabled, spies can sap robots on MvM");
#endif // SOURCE_ENGINE == SE_TF2


bool CTF2BotSpySapMvMRobotTask::IsPossible(CTF2Bot* bot)
{
#if SOURCE_ENGINE == SE_TF2
	// Guess what, another feature that's broken on RED bots.
	if (!cvar_allow_sapping_robots.GetBool())
	{
		return false;
	}
#endif // SOURCE_ENGINE == SE_TF2

	CBaseEntity* weapon = bot->GetWeaponOfSlot(static_cast<int>(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Secondary));

	if (!weapon)
	{
		return false;
	}

	float regenTime = 0.0f;

	if (!entprops->GetEntPropFloat(UtilHelpers::IndexOfEntity(weapon), Prop_Send, "m_flEffectBarRegenTime", regenTime))
	{
		return false;
	}

	if (regenTime >= gpGlobals->curtime)
	{
		return false;
	}

	if (!bot->GetUpgradeManager().HasBoughtUpgrade("robo sapper"))
	{
		return false;
	}

	return true;
}

CTF2BotSpySapMvMRobotTask::CTF2BotSpySapMvMRobotTask(CBaseEntity* target) :
	m_nav(CChaseNavigator::DONT_LEAD_SUBJECT), m_target(target)
{
	m_flEffectBarRegenTime = nullptr;
}

TaskResult<CTF2Bot> CTF2BotSpySapMvMRobotTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	bot->DelayedFakeClientCommand("build 3 0");

	CBaseEntity* weapon = bot->GetWeaponOfSlot(static_cast<int>(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Secondary));

	if (!weapon)
	{
		return Done("No sapper!");
	}

	m_flEffectBarRegenTime = entprops->GetPointerToEntData<float>(weapon, Prop_Send, "m_flEffectBarRegenTime");

	if (!m_flEffectBarRegenTime)
	{
		return Done("Sapper is invalid!");
	}

	if (bot->IsCloaked())
	{
		bot->GetControlInterface()->PressSecondaryAttackButton();
	}

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotSpySapMvMRobotTask::OnTaskUpdate(CTF2Bot* bot)
{
	CBaseEntity* target = m_target.Get();

	if (target == nullptr)
	{
		return Done("Target is NULL!");
	}
	
	if (!UtilHelpers::IsEntityAlive(target))
	{
		return Done("Target is dead!");
	}

	if (tf2lib::IsPlayerInCondition(target, TeamFortress2::TFCond::TFCond_Sapped) || *m_flEffectBarRegenTime >= gpGlobals->curtime)
	{
		return Done("Sapped!");
	}

	if (!bot->IsDisguised())
	{
		bot->DisguiseAs(TeamFortress2::TFClass_Engineer, false);
	}

	auto weapon = bot->GetInventoryInterface()->GetActiveBotWeapon();

	// the sapper uses slot 1
	if (weapon && weapon->GetWeaponInfo()->GetSlot() != static_cast<int>(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Secondary))
	{
		// equip sapper
		bot->DelayedFakeClientCommand("build 3 0");
	}

	bot->GetControlInterface()->SetDesiredAimSpot(IDecisionQuery::DesiredAimSpot::AIMSPOT_CENTER);
	bot->GetControlInterface()->AimAt(target, IPlayerController::LOOK_PRIORITY, 1.0f, "Looking at robot to sap!");

	if (bot->GetRangeTo(target) <= 128.0f && bot->GetControlInterface()->IsAimOnTarget())
	{
		bot->GetControlInterface()->PressAttackButton();
	}

	CTF2BotPathCost cost(bot);
	m_nav.Update(bot, target, cost, nullptr);

	return Continue();
}
