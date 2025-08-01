#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/helpers.h>
#include <bot/tf2/tf2bot.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <bot/tf2/tasks/tf2bot_scenario_task.h>
#include "tf2bot_medic_crossbow_heal_task.h"
#include "tf2bot_medic_medieval_task.h"

AITask<CTF2Bot>* CTF2BotMedicMedievalTask::InitialNextTask(CTF2Bot* bot)
{
	return CTF2BotScenarioTask::SelectScenarioTask(bot, true);
}

TaskResult<CTF2Bot> CTF2BotMedicMedievalTask::OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask)
{
	m_crossbowHealTimer.Start(5.0f);
	m_amputatorHealTimer.Start(5.0f);
	return Continue();
}

TaskResult<CTF2Bot> CTF2BotMedicMedievalTask::OnTaskUpdate(CTF2Bot* bot)
{
	constexpr int THE_AMPUTATOR_ITEM_DEF_INDEX = 304;

	if (m_tauntTimer.HasStarted() && m_tauntTimer.IsElapsed())
	{
		m_tauntTimer.Invalidate();
		bot->SendVoiceCommand(TeamFortress2::VoiceCommandsID::VC_MEDIC); // medic call to notify allies of the amputator taunt
		bot->DoWeaponTaunt();
	}

	if (m_crossbowHealTimer.IsElapsed())
	{
		const CTF2BotWeapon* crossbow = bot->GetInventoryInterface()->GetTheCrusadersCrossbow();

		if (crossbow)
		{
			m_crossbowHealTimer.Start(3.0f);

			if (!crossbow->IsOutOfAmmo(bot))
			{
				CBaseEntity* healtarget = CTF2BotMedicCrossbowHealTask::IsPossible(bot, crossbow);

				if (healtarget)
				{
					return PauseFor(new CTF2BotMedicCrossbowHealTask(healtarget), "Healing with the crossbow!");
				}
			}
		}
		else
		{
			m_crossbowHealTimer.Start(180.0f);
		}
	}

	if (m_amputatorHealTimer.IsElapsed())
	{
		const CBotWeapon* amputator = bot->GetInventoryInterface()->FindWeaponByEconIndex(THE_AMPUTATOR_ITEM_DEF_INDEX);

		if (amputator)
		{
			m_amputatorHealTimer.Start(15.0f);
			
			CTF2BotMedicMedievalTask::CountInjuredAllies functor(bot);
			bot->GetSensorInterface()->ForEveryKnownAlly(functor);

			if (functor.injured_allies > 0)
			{
				bot->GetInventoryInterface()->EquipWeapon(amputator);
				m_tauntTimer.Start(0.5f);
			}
			else
			{
				// no allies nearby to heal
				m_amputatorHealTimer.Start(2.0f);
			}
		}
		else
		{
			m_amputatorHealTimer.Start(180.0f);
		}
	}

	return Continue();
}

QueryAnswerType CTF2BotMedicMedievalTask::ShouldAttack(CBaseBot* me, const CKnownEntity* them)
{
	if (m_tauntTimer.HasStarted() && !m_tauntTimer.IsElapsed())
	{
		return ANSWER_NO;
	}

	return ANSWER_UNDEFINED;
}

QueryAnswerType CTF2BotMedicMedievalTask::ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon)
{
	if (m_tauntTimer.HasStarted() && !m_tauntTimer.IsElapsed())
	{
		return ANSWER_NO;
	}

	return ANSWER_UNDEFINED;
}

void CTF2BotMedicMedievalTask::CountInjuredAllies::operator()(const CKnownEntity* known)
{
	constexpr float HEAL_RANGE = 400.0f;

	if (known->IsPlayer())
	{
		if (this->me->GetRangeTo(known->GetEntity()) <= HEAL_RANGE && tf2lib::GetPlayerHealthPercentage(known->GetIndex()) <= 0.7f)
		{
			this->injured_allies++;
		}
	}
}
