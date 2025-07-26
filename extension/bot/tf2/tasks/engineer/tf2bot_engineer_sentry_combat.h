#ifndef __NAVBOT_TF2_BOT_ENGINEER_SENTRY_COMBAT_H_
#define __NAVBOT_TF2_BOT_ENGINEER_SENTRY_COMBAT_H_

class CTF2BotEngineerSentryCombatTask : public AITask<CTF2Bot>
{
public:
	CTF2BotEngineerSentryCombatTask() :
		m_goal(0.0f, 0.0f, 0.0f)
	{
		m_hasWrangler = false;
	}

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;
	void OnTaskEnd(CTF2Bot* bot, AITask<CTF2Bot>* nextTask) override;

	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override;

	const char* GetName() const override { return "SentryCombat"; }
private:
	CMeshNavigatorAutoRepath m_nav;
	Vector m_goal;
	bool m_hasWrangler;

	static bool IsSentryLineOfFireClear(CBaseEntity* sentry, CBaseEntity* threat);

	static constexpr float BEHIND_SENTRY_DIST = 128.0f;
	static constexpr float TOO_CLOSE_FOR_WRANGLER = 400.0f;

};


#endif // !__NAVBOT_TF2_BOT_ENGINEER_SENTRY_COMBAT_H_
