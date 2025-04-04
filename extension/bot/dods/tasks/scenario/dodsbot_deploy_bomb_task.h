#ifndef __NAVBOT_DODSBOT_DEPLOY_BOMB_TASK_H_
#define __NAVBOT_DODSBOT_DEPLOY_BOMB_TASK_H_

class CDoDSBotDeployBombTask : public AITask<CDoDSBot>
{
public:
	CDoDSBotDeployBombTask(CBaseEntity* target);

	TaskResult<CDoDSBot> OnTaskStart(CDoDSBot* bot, AITask<CDoDSBot>* pastTask) override;
	TaskResult<CDoDSBot> OnTaskUpdate(CDoDSBot* bot) override;

	const char* GetName() const override { return "DeployBomb"; }
private:
	CHandle<CBaseEntity> m_target;
	CMeshNavigator m_nav;
	CountdownTimer m_repathtimer;
};


#endif // !__NAVBOT_DODSBOT_DEPLOY_BOMB_TASK_H_
