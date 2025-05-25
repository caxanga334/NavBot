#ifndef __NAVBOT_TF2BOT_RD_DESTROY_ROBOTS_TASK_H_
#define __NAVBOT_TF2BOT_RD_DESTROY_ROBOTS_TASK_H_

class CTF2BotRDDestroyRobotsTask : public AITask<CTF2Bot>
{
public:
	static bool IsPossible(CTF2Bot* bot, std::vector<CHandle<CBaseEntity>>& robots);
	CTF2BotRDDestroyRobotsTask(std::vector<CHandle<CBaseEntity>>&& robots);

	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	TaskEventResponseResult<CTF2Bot> OnOtherKilled(CTF2Bot* bot, CBaseEntity* pVictim, const CTakeDamageInfo& info) override;

	const char* GetName() const override { return "RDDestroyEnemyRobots"; }

private:
	std::vector<CHandle<CBaseEntity>> m_robots;
	CChaseNavigator m_nav;
	std::size_t m_iter;
};

#endif // !__NAVBOT_TF2BOT_RD_DESTROY_ROBOTS_TASK_H_
