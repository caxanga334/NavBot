#ifndef __NAVBOT_TF2BOT_SD_DELIVER_FLAG_TASK_H_
#define __NAVBOT_TF2BOT_SD_DELIVER_FLAG_TASK_H_

class CTF2BotSDDeliverFlag : public AITask<CTF2Bot>
{
public:
	CTF2BotSDDeliverFlag() :
		m_goal(0.0f, 0.0f, 0.0f)
	{
		m_capwasdisabled = false;
	}

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	// Don't retreat or seek enemies while delivering the australium
	QueryAnswerType ShouldSeekAndDestroy(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_NO; }
	QueryAnswerType ShouldHurry(CBaseBot* me) override { return ANSWER_YES; }
	QueryAnswerType ShouldRetreat(CBaseBot* me) override { return ANSWER_NO; }

	const char* GetName() const override { return "DeliverAustralium"; }

private:
	Vector m_goal;
	CMeshNavigator m_nav;
	CHandle<CBaseEntity> m_capzone;
	CHandle<CBaseEntity> m_flagzone;
	bool m_capwasdisabled;
};

#endif // !__NAVBOT_TF2BOT_SD_DELIVER_FLAG_TASK_H_
