#include NAVBOT_PCH_FILE
#include <mods/css/nav/css_nav_mesh.h>
#include <mods/css/css_lib.h>
#include <mods/css/css_mod.h>
#include <bot/css/cssbot.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <bot/tasks_shared/bot_shared_defend_spot.h>
#include <bot/tasks_shared/bot_shared_go_to_position.h>
#include <bot/tasks_shared/bot_shared_take_cover_from_spot.h>
#include "cssbot_defend_planted_bomb_task.h"

AITask<CCSSBot>* CCSSBotDefendPlantedBombTask::InitialNextTask(CCSSBot* bot)
{
	CBaseEntity* c4 = CCounterStrikeSourceMod::GetCSSMod()->GetActiveBombEntity();

	if (c4)
	{
		return new CBotSharedDefendSpotTask<CCSSBot, CCSSBotPathCost>(bot, UtilHelpers::getEntityOrigin(c4), csslib::GetC4TimeRemaining(c4), true);
	}

	return nullptr;
}

TaskResult<CCSSBot> CCSSBotDefendPlantedBombTask::OnTaskUpdate(CCSSBot* bot)
{
	CBaseEntity* c4 = CCounterStrikeSourceMod::GetCSSMod()->GetActiveBombEntity();

	if (!c4)
	{
		return Done("C4 is NULL!");
	}

	const Vector& pos = UtilHelpers::getEntityOrigin(c4);

	if (csslib::IsC4BeingDefused(c4))
	{
		return PauseFor(new CBotSharedGoToPositionTask<CCSSBot, CCSSBotPathCost>(bot, pos, "ProtectC4", true, true, true), "C4 is being defused!");
	}

	const float timetoexplode = csslib::GetC4TimeRemaining(c4);

	if (timetoexplode < 5.0f)
	{
		const CBotWeapon* knife = bot->GetInventoryInterface()->FindWeaponBySlot(counterstrikesource::CSS_WEAPON_SLOT_KNIFE);

		if (knife)
		{
			bot->GetInventoryInterface()->EquipWeapon(knife);
		}

		return SwitchTo(new CBotSharedTakeCoverFromSpotTask<CCSSBot, CCSSBotPathCost>(bot, pos, 2048.0f, false, false, 10000.0f), "Running away from exploding bomb!");
	}

	// if the defend spot task ended, end this one too and let scenario pick a new one.
	if (GetNextTask() == nullptr)
	{
		return Done("Done defending!");
	}

	return Continue();
}
