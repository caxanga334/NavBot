#ifndef __NAVBOT_TF2BOT_DESTROY_HALLOWEEN_BOSS_TASK_H_
#define __NAVBOT_TF2BOT_DESTROY_HALLOWEEN_BOSS_TASK_H_

class CTF2BotDestroyHalloweenBossTask : public AITask<CTF2Bot>
{
public:
	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	const char* GetName() const override { return "DestroyHalloweenBoss"; }
private:
	CHandle<CBaseEntity> m_boss;
	Vector m_goal;
	CMeshNavigator m_nav;

	bool FindHalloweenBoss();
	void UpdateBossPosition(CBaseEntity* boss);
};

#endif // !__NAVBOT_TF2BOT_DESTROY_HALLOWEEN_BOSS_TASK_H_
