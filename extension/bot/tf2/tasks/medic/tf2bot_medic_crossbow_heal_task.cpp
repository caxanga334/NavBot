#include NAVBOT_PCH_FILE
#include <bot/tf2/tf2bot.h>
#include <bot/interfaces/weapon.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include "tf2bot_medic_crossbow_heal_task.h"

CBaseEntity* CTF2BotMedicCrossbowHealTask::IsPossible(CTF2Bot* me, const CBotWeapon* crossbow)
{
	std::array<CBaseEntity*, ABSOLUTE_PLAYER_LIMIT> targets;
	std::fill(targets.begin(), targets.end(), nullptr);
	std::size_t target_count = 0U;
	const TeamFortress2::TFTeam myteam = me->GetMyTFTeam();
	const float min_range = CTeamFortress2Mod::GetTF2Mod()->IsPlayingMedievalMode() ? 128.0f : 600.0f;

	auto functor = [&me, &crossbow, &targets, &target_count, &myteam, min_range](int client, edict_t* entity, SourceMod::IGamePlayer* player) -> void {

		if (target_count >= targets.max_size())
		{
			return;
		}

		if (tf2lib::GetEntityTFTeam(client) == myteam || tf2lib::GetPlayerClassType(client) == TeamFortress2::TFClassType::TFClass_Spy)
		{
			if (tf2lib::GetPlayerHealthPercentage(client) > 0.7f)
			{
				return;
			}

			const float range = me->GetRangeTo(entity);

			if (range > min_range && range <= crossbow->GetWeaponInfo()->GetAttackInfo(WeaponInfo::AttackFunctionType::PRIMARY_ATTACK).GetMaxRange())
			{
				if (me->IsLineOfFireClear(UtilHelpers::getWorldSpaceCenter(entity)))
				{
					targets[target_count] = UtilHelpers::EdictToBaseEntity(entity);
					target_count++;
				}
			}
		}
	};

	UtilHelpers::ForEachPlayer(functor);

	if (target_count == 0U)
	{
		return nullptr;
	}
	else if (target_count == 1U)
	{
		return targets[0];
	}

	return targets[randomgen->GetRandomInt<std::size_t>(0U, target_count - 1U)];
}

TaskResult<CTF2Bot> CTF2BotMedicCrossbowHealTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	CBaseEntity* heal = m_healTarget.Get();

	if (!heal)
	{
		return Done("Heal target is invalid!");
	}

	const CBotWeapon* crossbow = bot->GetInventoryInterface()->FindWeaponByClassname("tf_weapon_crossbow");

	if (!crossbow)
	{
		return Done("I don't have a crossbow!");
	}

	bot->SelectWeapon(crossbow->GetEntity());
	m_giveupTimer.Start(10.0f);
	m_aimTimer.Start(1.0f);

	return Continue();
}

TaskResult<CTF2Bot> CTF2BotMedicCrossbowHealTask::OnTaskUpdate(CTF2Bot* bot)
{
	if (m_giveupTimer.IsElapsed())
	{
		return Done("Give up timer elapsed!");
	}

	CBaseEntity* heal = m_healTarget.Get();

	if (!heal)
	{
		return Done("Heal target is invalid!");
	}

	if (tf2lib::GetPlayerHealthPercentage(m_healTarget.GetEntryIndex()) >= 0.95f)
	{
		return Done("Target healed!");
	}

	auto control = bot->GetControlInterface();

	control->SetDesiredAimSpot(IDecisionQuery::DesiredAimSpot::AIMSPOT_CENTER);
	control->AimAt(heal, IPlayerController::LOOK_PRIORITY, 1.0f, "Aiming the crossbow on the heal target!");

	const CBotWeapon* crossbow = bot->GetInventoryInterface()->FindWeaponByClassname("tf_weapon_crossbow");

	if (!crossbow)
	{
		return Done("I don't have a crossbow!");
	}

	if (m_aimTimer.IsElapsed())
	{
		if (control->IsAimOnTarget())
		{
			if (crossbow->GetBaseCombatWeapon().GetClip1() >= 1)
			{
				control->PressAttackButton(0.1f);
			}
			else
			{
				control->PressReloadButton();
			}
		}
	}

	return Continue();
}
