#ifndef __NAVBOT_TF2BOT_RD_COLLECT_POINTS_TASK_H_
#define __NAVBOT_TF2BOT_RD_COLLECT_POINTS_TASK_H_

class CTF2BotRDCollectPointsTask : public AITask<CTF2Bot>
{
public:
	static bool IsPossible(CTF2Bot* bot, std::vector<CHandle<CBaseEntity>>& points);
	CTF2BotRDCollectPointsTask(std::vector<CHandle<CBaseEntity>>&& points);

	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	const char* GetName() const override { return "RDCollectPoints"; }

private:
	std::vector<CHandle<CBaseEntity>> m_points;
	CChaseNavigator m_nav;
	std::size_t m_iter;
};

#endif // !__NAVBOT_TF2BOT_RD_COLLECT_POINTS_TASK_H_
