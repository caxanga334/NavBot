#ifndef __NAVBOT_TF2BOT_DEMOMAN_LAY_STICKY_TRAP_TASK_H_
#define __NAVBOT_TF2BOT_DEMOMAN_LAY_STICKY_TRAP_TASK_H_

class CTF2BotDemomanLayStickyTrapTask : public AITask<CTF2Bot>
{
public:
	CTF2BotDemomanLayStickyTrapTask(const Vector& trapPos);

	static bool IsPossible(CTF2Bot* bot);

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	TaskEventResponseResult<CTF2Bot> OnInjured(CTF2Bot* bot, const CTakeDamageInfo& info) override;
	TaskEventResponseResult<CTF2Bot> OnMoveToSuccess(CTF2Bot* bot, CPath* path) override;

	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_NO; }
	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override { return ANSWER_NO; }
	QueryAnswerType ShouldRetreat(CBaseBot* me) override { return ANSWER_NO; }

	const char* GetName() const override { return "LayStickyTrap"; }

private:
	CMeshNavigator m_nav;
	Vector m_trapLocation;
	Vector m_hideSpot;
	Vector m_aimPos;
	bool m_justFired;
	bool m_trapDeployed;
	int m_maxBombs;
	CountdownTimer m_reloadWaitTimer;
	CountdownTimer m_trapEndTimer;

	void RandomAimSpot();
	Vector SelectHidingSpot(CTF2Bot* bot) const;
};


#endif // !__NAVBOT_TF2BOT_DEMOMAN_LAY_STICKY_TRAP_TASK_H_
