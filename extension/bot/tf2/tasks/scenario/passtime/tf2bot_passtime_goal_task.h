#ifndef __NAVBOT_TF2BOT_PASSTIME_GOAL_TASK_H_
#define __NAVBOT_TF2BOT_PASSTIME_GOAL_TASK_H_

class CTF2BotPassTimeGoalTask : public AITask<CTF2Bot>
{
public:

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;
	void OnTaskEnd(CTF2Bot* bot, AITask<CTF2Bot>* nextTask) override;

	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_NO; }
	QueryAnswerType ShouldSeekAndDestroy(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_NO; }
	QueryAnswerType ShouldHurry(CBaseBot* me) override { return ANSWER_YES; }
	QueryAnswerType ShouldRetreat(CBaseBot* me) override { return ANSWER_NO; }
	QueryAnswerType ShouldAssistTeammate(CBaseBot* me, CBaseEntity* teammate) override { return ANSWER_NO; }
	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override { return ANSWER_NO; }

	const char* GetName() const override { return "GoalJack"; }

private:
	CHandle<CBaseEntity> m_goalent;
	CMeshNavigator m_nav;
	CountdownTimer m_repathtimer;
	TeamFortress2::PassTimeGoalType m_goaltype;
	CountdownTimer m_holdattacktimer;

	static constexpr float MAX_RANGE_TO_LAUNCH_JACK = 512.0f;
	bool NeedsToFire() const;
	float GetZ(const Vector& origin, const Vector& center) const;
};

#endif // !__NAVBOT_TF2BOT_PASSTIME_GOAL_TASK_H_
