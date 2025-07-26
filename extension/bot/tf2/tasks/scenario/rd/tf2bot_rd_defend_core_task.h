#ifndef __NAVBOT_TF2BOT_RD_DEFEND_CORE_TASK_H_
#define __NAVBOT_TF2BOT_RD_DEFEND_CORE_TASK_H_

class CTF2BotRDDefendCoreTask : public AITask<CTF2Bot>
{
public:
	static bool IsPossible(CTF2Bot* bot, CBaseEntity** core);
	CTF2BotRDDefendCoreTask(CBaseEntity* core);
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	const char* GetName() const override { return "RDDefendReactorCore"; }

private:
	CHandle<CBaseEntity> m_flag;
	CMeshNavigator m_nav;
};

#endif // !__NAVBOT_TF2BOT_RD_DEFEND_CORE_TASK_H_
