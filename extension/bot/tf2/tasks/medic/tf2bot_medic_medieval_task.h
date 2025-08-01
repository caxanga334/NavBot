#ifndef __NAVBOT_TF2BOT_MEDIC_MEDIEVAL_TASK_H_
#define __NAVBOT_TF2BOT_MEDIC_MEDIEVAL_TASK_H_

// Medic behavior for medieval mode
class CTF2BotMedicMedievalTask : public AITask<CTF2Bot>
{
public:
	AITask<CTF2Bot>* InitialNextTask(CTF2Bot* bot) override;
	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override;
	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override;

	const char* GetName() const override { return "MedicMedieval"; }

private:
	CountdownTimer m_crossbowHealTimer;
	CountdownTimer m_amputatorHealTimer;
	CountdownTimer m_tauntTimer;

	class CountInjuredAllies
	{
	public:
		CountInjuredAllies(CTF2Bot* bot)
		{
			me = bot;
			injured_allies = 0;
		}

		void operator()(const CKnownEntity* known);

		CTF2Bot* me;
		int injured_allies;
	};
};


#endif // !__NAVBOT_TF2BOT_MEDIC_MEDIEVAL_TASK_H_
