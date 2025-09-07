#ifndef __NAVBOT_TF2BOT_ZOMBIE_INFECTION_TASKS_H_
#define __NAVBOT_TF2BOT_ZOMBIE_INFECTION_TASKS_H_

class CTF2BotZIMonitorTask : public AITask<CTF2Bot>
{
public:
	CTF2BotZIMonitorTask();

	AITask<CTF2Bot>* InitialNextTask(CTF2Bot* bot) override;

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	const char* GetName() const override { return "ZombieInfection"; }
private:
	bool m_isUsingClassBehavior;
	bool m_isZombie;
	CountdownTimer m_zombieThinkTimer;
	CountdownTimer m_zombieAbilityCooldown;

	void DetectRevealedPlayers(CTF2Bot* me) const;
	bool ShouldUseAbility(CTF2Bot* me) const;
};

class CTF2BotZISurvivorBehaviorTask : public AITask<CTF2Bot>
{
public:
	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	const char* GetName() const override { return "ZISurvivor"; }
private:

};

class CTF2BotZIZombieBehaviorTask : public AITask<CTF2Bot>
{
public:
	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	QueryAnswerType ShouldHurry(CBaseBot* me) override { return ANSWER_YES; }
	QueryAnswerType ShouldRetreat(CBaseBot* me) override { return ANSWER_NO; }

	const char* GetName() const override { return "ZIZombie"; }
private:

};


#endif // !__NAVBOT_TF2BOT_ZOMBIE_INFECTION_TASKS_H_
