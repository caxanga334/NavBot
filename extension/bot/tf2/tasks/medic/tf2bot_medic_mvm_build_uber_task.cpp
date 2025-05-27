#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <sdkports/sdk_ehandle.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <entities/tf2/tf_entities.h>
#include <bot/interfaces/path/meshnavigator.h>
#include "tf2bot_medic_mvm_build_uber_task.h"

TaskResult<CTF2Bot> CTF2BotMedicMvMBuildUberTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	CBaseEntity* medigun = bot->GetWeaponOfSlot(static_cast<int>(TeamFortress2::TFWeaponSlot::TFWeaponSlot_Secondary));

	if (!medigun || !UtilHelpers::FClassnameIs(medigun, "tf_weapon_medigun"))
	{
		return Done("No medigun!");
	}

	m_flChargeLevel = entprops->GetPointerToEntData<float>(medigun, Prop_Send, "m_flChargeLevel");
	m_bHealing = entprops->GetPointerToEntData<bool>(medigun, Prop_Send, "m_bHealing");
	m_hHealingTarget = entprops->GetPointerToEntData<CHandle<CBaseEntity>>(medigun, Prop_Send, "m_hHealingTarget");

	if (!FindPlayerToHeal(bot))
	{
		return Done("Nobody to heal!");
	}

	bot->SelectWeapon(medigun);

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotMedicMvMBuildUberTask::OnTaskUpdate(CTF2Bot* bot)
{
	CBaseEntity* target = m_healTarget.Get();

	if (!target)
	{
		if (!FindPlayerToHeal(bot))
		{
			return Done("Nobody to heal!");
		}

		target = m_healTarget.Get();
	}
	
	if (*m_flChargeLevel >= 1.0f)
	{
		return Done("Uber built!");
	}

	constexpr float MEDIGUN_RANGE = 400.0f;

	if (bot->GetRangeTo(target) <= MEDIGUN_RANGE)
	{
		bot->GetControlInterface()->SetDesiredAimSpot(IDecisionQuery::DesiredAimSpot::AIMSPOT_CENTER);
		bot->GetControlInterface()->AimAt(target, IPlayerController::LOOK_SUPPORT, 2.0f, "Looking at my heal target!");

		if (bot->GetControlInterface()->IsAimOnTarget())
		{
			bot->GetControlInterface()->PressAttackButton(1.0f);
		}
	}
	else
	{
		bot->GetControlInterface()->ReleaseAttackButton();
		CTF2BotPathCost cost(bot);
		const Vector& goal = UtilHelpers::getEntityOrigin(target);
		m_nav.Update<CTF2BotPathCost>(bot, goal, cost);
	}

	return Continue();
}

bool CTF2BotMedicMvMBuildUberTask::FindPlayerToHeal(CTF2Bot* me)
{
	edict_t* target = nullptr;
	m_healTarget.Term();

	auto func = [&me, &target](int client, edict_t* entity, SourceMod::IGamePlayer* player) -> void {
		if (!target && player->IsInGame() && !player->GetPlayerInfo()->IsDead())
		{
			if (client == me->GetIndex())
			{
				return;
			}

			int teamnum = 0;
			entprops->GetEntProp(client, Prop_Send, "m_iTeamNum", teamnum);

			if (teamnum == static_cast<int>(me->GetMyTFTeam()))
			{
				target = entity;
				return;
			}
		}
	};

	UtilHelpers::ForEachPlayer(func);

	if (target)
	{
		m_healTarget = UtilHelpers::EdictToBaseEntity(target);
	}

	return m_healTarget.IsValid();
}
