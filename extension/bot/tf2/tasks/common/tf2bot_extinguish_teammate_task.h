#ifndef __NAVBOT_TF2BOT_COMMON_EXTINGUISH_TEAMMATE_TASKS_H_
#define __NAVBOT_TF2BOT_COMMON_EXTINGUISH_TEAMMATE_TASKS_H_

class CTF2BotExtinguishTeammateTask : public AITask<CTF2Bot>
{
public:
	static bool IsPossible(CTF2Bot* bot, CBaseEntity** teammate, CBaseEntity** weapon);

	CTF2BotExtinguishTeammateTask(CBaseEntity* teammate, CBaseEntity* weapon);

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	TaskEventResponseResult<CTF2Bot> OnStuck(CTF2Bot* bot) override { return TryToMaintain(PRIORITY_HIGH); }
	TaskEventResponseResult<CTF2Bot> OnUnstuck(CTF2Bot* bot) override { return TryToMaintain(PRIORITY_HIGH); }
	TaskEventResponseResult<CTF2Bot> OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason) override { return TryToMaintain(PRIORITY_HIGH); }
	TaskEventResponseResult<CTF2Bot> OnMoveToSuccess(CTF2Bot* bot, CPath* path) override { return TryToMaintain(PRIORITY_HIGH); }

	QueryAnswerType ShouldHurry(CBaseBot* me) override { return ANSWER_YES; }

	const char* GetName() const override { return "SpyCheck"; }
private:
	CChaseNavigator m_nav;
	CHandle<CBaseEntity> m_teammate;
	CHandle<CBaseEntity> m_weapon;
	CountdownTimer m_timeout;
	bool m_isAirblast;

	const CBotWeapon* GetWeapon(const CTF2Bot* bot) const;
};


#endif // !__NAVBOT_TF2BOT_COMMON_EXTINGUISH_TEAMMATE_TASKS_H_
