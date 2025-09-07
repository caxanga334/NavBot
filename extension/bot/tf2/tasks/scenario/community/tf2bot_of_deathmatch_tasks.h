#ifndef __NAVBOT_TF2BOT_OF_DEATHMATCH_TASKS_H_
#define __NAVBOT_TF2BOT_OF_DEATHMATCH_TASKS_H_

/* AI Tasks for the Open Fortress style Free for All Deathmatch game mode (vscript) */

class CTF2BotOFDMMonitorTask : public AITask<CTF2Bot>
{
public:
	AITask<CTF2Bot>* InitialNextTask(CTF2Bot* bot) override;

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	// Block the default weapon switch
	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override { return ANSWER_NO; }

	const char* GetName() const override { return "OFDMMonitor"; }
private:
	CountdownTimer m_weaponSelectCooldownTimer;
	CountdownTimer m_collectWeaponTimer;

	void SelectRandomWeapon(CTF2Bot* bot);
	CBaseEntity* CollectRandomWeapon(CTF2Bot* bot) const;
};

#endif // !__NAVBOT_TF2BOT_OF_DEATHMATCH_TASKS_H_
