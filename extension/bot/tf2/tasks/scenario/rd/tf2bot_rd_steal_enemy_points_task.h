#ifndef __NAVBOT_TF2BOT_RD_STEAL_ENEMY_POINTS_TASK_H_
#define __NAVBOT_TF2BOT_RD_STEAL_ENEMY_POINTS_TASK_H_

class CTF2BotRDStealEnemyPointsTask : public AITask<CTF2Bot>
{
public:
	static bool IsPossible(CTF2Bot* bot, CBaseEntity** flag);

	CTF2BotRDStealEnemyPointsTask(CBaseEntity* flag);

	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	const char* GetName() const override { return "RDStealEnemyPoints"; }

private:
	CHandle<CBaseEntity> m_flag;
	CMeshNavigator m_nav;
	CountdownTimer m_repathtimer;
	Vector m_homepos;
	bool m_washome;
};

#endif // !__NAVBOT_TF2BOT_RD_STEAL_ENEMY_POINTS_TASK_H_
