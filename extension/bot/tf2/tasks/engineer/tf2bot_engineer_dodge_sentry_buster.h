#ifndef __NAVBOT_TF2BOT_ENGINEER_MVM_DODGE_SENTRY_BUSTER_H_
#define __NAVBOT_TF2BOT_ENGINEER_MVM_DODGE_SENTRY_BUSTER_H_

class CTF2BotEngineerMvMDodgeSentryBusterTask : public AITask<CTF2Bot>
{
public:
	static bool IsPossible(CTF2Bot* bot, CBaseEntity** sentryBuster);

	CTF2BotEngineerMvMDodgeSentryBusterTask(CBaseEntity* sentryBuster);

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	QueryAnswerType ShouldHurry(CBaseBot* me) override { return ANSWER_YES; }
	QueryAnswerType ShouldRetreat(CBaseBot* me) override { return ANSWER_NO; }
	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_NO; }
	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override { return ANSWER_NO; }

	const char* GetName() const override { return "EngineerDodgeSentryBuster"; }
private:
	CHandle<CBaseEntity> m_sentryBuster;
	bool m_hasSentry;
	bool m_detonating;
	CountdownTimer m_repathTimer;
	CMeshNavigator m_nav;
	Vector m_goal;
	float m_detonationRange;

	// if a sentry buster is this close to a sentry, it's a problem
	static constexpr auto SENTRY_BUSTER_DANGER_RADIUS = 1500.0f;
	static constexpr auto SENTRY_BUSTER_MIN_COVER_RADIUS = 700.0f;
	static constexpr auto SENTRY_BUSTER_MAX_COVER_RADIUS = 2048.0f;
	static constexpr auto SENTRY_BUSTER_DEFAULT_DETONATION_RANGE = 100.0f;
};


#endif // !__NAVBOT_TF2BOT_ENGINEER_MVM_DODGE_SENTRY_BUSTER_H_
