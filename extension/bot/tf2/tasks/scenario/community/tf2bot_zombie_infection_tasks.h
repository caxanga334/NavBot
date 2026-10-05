#ifndef __NAVBOT_TF2BOT_ZOMBIE_INFECTION_TASKS_H_
#define __NAVBOT_TF2BOT_ZOMBIE_INFECTION_TASKS_H_

class CTF2BotZIZombieRespawningTask : public AITask<CTF2Bot>
{
public:
	CTF2BotZIZombieRespawningTask()
	{
		m_didJump = false;
	}

	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	QueryAnswerType ShouldHurry(CBaseBot* me) override { return ANSWER_YES; }
	QueryAnswerType ShouldRetreat(CBaseBot* me) override { return ANSWER_NO; }

	const char* GetName() const override { return "ZombieRespawning"; }
private:
	bool m_didJump;
};

class CTF2BotZIMonitorTask : public AITask<CTF2Bot>
{
public:
	CTF2BotZIMonitorTask();

	AITask<CTF2Bot>* InitialNextTask(CTF2Bot* bot) override;

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	QueryAnswerType ShouldHurry(CBaseBot* me) override
	{
		if (m_isZombie)
		{
			return ANSWER_YES;
		}

		return ANSWER_UNDEFINED;
	}

	QueryAnswerType ShouldRetreat(CBaseBot* me) override
	{
		if (m_isZombie)
		{
			return ANSWER_NO;
		}

		return ANSWER_UNDEFINED;
	}

	const char* GetName() const override { return "ZombieInfection"; }
private:
	bool m_isUsingClassBehavior;
	bool m_isZombie;
	bool m_placedBeacon;
	CountdownTimer m_zombieThinkTimer;
	CountdownTimer m_zombieAbilityCooldown;

	void DetectRevealedPlayers(CTF2Bot* me) const;
	bool ShouldUseAbility(CTF2Bot* me) const;
	bool AllowedToBuildBeacon(CTF2Bot* me) const;
	bool FindBeaconBuildSpot(CTF2Bot* me, Vector& spot) const;
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

class CTF2BotZIBuildBeaconTask : public AITask<CTF2Bot>
{
public:
	CTF2BotZIBuildBeaconTask(const Vector& buildSpot) :
		m_buildSpot(buildSpot)
	{
	}

	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	TaskEventResponseResult<CTF2Bot> OnStuck(CTF2Bot* bot) override { return TryToMaintain(PRIORITY_HIGH); }
	TaskEventResponseResult<CTF2Bot> OnUnstuck(CTF2Bot* bot) override { return TryToMaintain(PRIORITY_HIGH); }
	TaskEventResponseResult<CTF2Bot> OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason) override { return TryToMaintain(PRIORITY_HIGH); }
	TaskEventResponseResult<CTF2Bot> OnMoveToSuccess(CTF2Bot* bot, CPath* path) override { return TryToMaintain(PRIORITY_HIGH); }

	QueryAnswerType ShouldHurry(CBaseBot* me) override { return ANSWER_YES; }
	QueryAnswerType ShouldRetreat(CBaseBot* me) override { return ANSWER_NO; }

	const char* GetName() const override { return "BuildSpawnBeacon"; }
private:
	Vector m_buildSpot;
	CMeshNavigator m_nav;
	CPathFailCounter m_counter;

	bool IsLOFClear(CTF2Bot* bot) const;
};

#endif // !__NAVBOT_TF2BOT_ZOMBIE_INFECTION_TASKS_H_
