#ifndef __NAVBOT_TF2BOT_PD_COLLECT_TASK_H_
#define __NAVBOT_TF2BOT_PD_COLLECT_TASK_H_

class CTF2BotPDCollectTask : public AITask<CTF2Bot>
{
public:
	CTF2BotPDCollectTask(std::vector<CHandle<CBaseEntity>>&& toCollect);

	static bool IsPossible(CTF2Bot* bot, std::vector<CHandle<CBaseEntity>>& points);

	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	QueryAnswerType ShouldSeekAndDestroy(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_NO; }
	QueryAnswerType ShouldHurry(CBaseBot* me) override { return ANSWER_YES; }
	QueryAnswerType ShouldRetreat(CBaseBot* me) override { return ANSWER_NO; }
	QueryAnswerType ShouldAssistTeammate(CBaseBot* me, CBaseEntity* teammate) override { return ANSWER_NO; }

	const char* GetName() const override { return "PDCollect"; }
private:
	std::vector<CHandle<CBaseEntity>> m_points;
	std::size_t m_index;
	CMeshNavigator m_nav;
};

#endif // !__NAVBOT_TF2BOT_PD_COLLECT_TASK_H_
