#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/entprops.h>
#include <bot/dods/dodsbot.h>
#include <mods/dods/dayofdefeatsourcemod.h>
#include <sdkports/sdk_takedamageinfo.h>
#include <bot/tasks_shared/bot_shared_prereq_destroy_ent.h>
#include <bot/tasks_shared/bot_shared_prereq_move_to_pos.h>
#include <bot/tasks_shared/bot_shared_prereq_use_ent.h>
#include <bot/tasks_shared/bot_shared_prereq_wait.h>
#include <bot/interfaces/behavior_utils.h>
#include "dodsbot_scenario_monitor_task.h"
#include "dodsbot_tactical_monitor_task.h"

AITask<CDoDSBot>* CDoDSBotTacticalMonitorTask::InitialNextTask(CDoDSBot* bot)
{
	return new CDoDSBotScenarioMonitorTask;
}

/**
 * @brief Checks whether the bot's active weapon would benefit from a reload right now.
 *
 * Returns true only when:
 *   - The weapon uses a clip (not a reserve-only ammo pool)
 *   - The bot is not completely out of ammo
 *   - The clip is empty, OR the clip is below max capacity AND reserve ammo exists to refill it
 *
 * The max clip size is read from the engine's data map (m_iMaxClip1). If that read fails,
 * the function conservatively only returns true for an empty clip to avoid spurious reloads.
 *
 * @param weapon  The bot's currently active weapon.
 * @param bot     The bot that owns the weapon.
 * @return True if pressing reload would be useful right now.
 */
static bool NeedsReload(const CDoDSBotWeapon* weapon, const CBaseBot* bot)
{
	if (!weapon || !weapon->IsValid())
		return false;

	const WeaponInfo* info = weapon->GetWeaponInfo();
	if (!info)
		return false;

	const auto& bcw = weapon->GetBaseCombatWeapon();

	// Weapon treats all ammo as a reserve pool with no clip (e.g. rocket launcher).
	// Pressing reload does nothing for these weapons.
	if (info->Clip1IsReserveAmmo() || !bcw.UsesClipsForAmmo1())
		return false;

	// Completely out of ammo — nothing to reload from.
	if (weapon->IsOutOfAmmo(bot))
		return false;

	int clip = bcw.GetClip1();

	// Clip is empty but we have reserve ammo — reload immediately.
	if (clip == 0)
		return true;

	// Clip has rounds remaining — only worth reloading if there is reserve ammo to top up from.
	int reserveType = bcw.GetPrimaryAmmoType();
	if (reserveType > 0 && bot->GetAmmoOfIndex(reserveType) <= 0)
		return false;

	// Read the weapon's max clip size from the engine data map.
	// This is not a networked property; Prop_Data is required.
	int maxClip = -1;
	entprops->GetEntProp(weapon->GetIndex(), Prop_Data, "m_iMaxClip1", maxClip);

	if (maxClip > 0)
		return clip < maxClip; // reload if the clip is not at full capacity

	// Could not read max clip — only reload on empty (already handled above).
	return false;
}

TaskResult<CDoDSBot> CDoDSBotTacticalMonitorTask::OnTaskUpdate(CDoDSBot* bot)
{
	// Tactical reload: reload the active weapon when out of combat.
	// Checked once per second to avoid spamming the reload button.
	if (m_reloadCheckTimer.IsElapsed())
	{
		m_reloadCheckTimer.Start(1.0f);

		const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat();
		bool recentThreat = (threat != nullptr) && threat->WasRecentlyVisible(5.0f);

		if (!recentThreat)
		{
			const CDoDSBotWeapon* weapon = bot->GetInventoryInterface()->GetActiveDoDWeapon();
			if (NeedsReload(weapon, bot))
			{
				bot->GetControlInterface()->PressReloadButton();
			}
		}
	}

	return Continue();
}

TaskEventResponseResult<CDoDSBot> CDoDSBotTacticalMonitorTask::OnNavAreaChanged(CDoDSBot* bot, CNavArea* oldArea, CNavArea* newArea)
{
	BOTBEHAVIOR_IMPLEMENT_PREREQUISITE_CHECK(CDoDSBot, CDoDSBotPathCost);

	return TryContinue(PRIORITY_LOW);
}

TaskEventResponseResult<CDoDSBot> CDoDSBotTacticalMonitorTask::OnInjured(CDoDSBot* bot, const CTakeDamageInfo& info)
{
	if (bot->GetDifficultyProfile()->GetGameAwareness() >= 15)
	{
		CBaseEntity* attacker = info.GetAttacker();

		if (attacker)
		{
			Vector pos = UtilHelpers::getWorldSpaceCenter(attacker);
			bot->GetControlInterface()->AimAt(pos, IPlayerController::LOOK_DANGER, 1.5f, "Looking at attacker!");

			if (UtilHelpers::IsPlayer(attacker))
			{
				CKnownEntity* known = bot->GetSensorInterface()->AddKnownEntity(attacker);
				known->UpdatePosition();
			}
		}
	}

	return TryContinue(PRIORITY_LOW);
}
