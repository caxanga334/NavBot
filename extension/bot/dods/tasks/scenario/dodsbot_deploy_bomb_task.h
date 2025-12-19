#ifndef __NAVBOT_DODSBOT_DEPLOY_BOMB_TASK_H_
#define __NAVBOT_DODSBOT_DEPLOY_BOMB_TASK_H_

class CDoDSBotDeployBombTask : public AITask<CDoDSBot>
{
public:
	CDoDSBotDeployBombTask(CBaseEntity* target, const bool isObjective = true);

	static constexpr int IDENTIFIER = 0xD0DB; // task identifier
	static constexpr int FLAG_IS_OBJECTIVE = 0x8;

	TaskResult<CDoDSBot> OnTaskStart(CDoDSBot* bot, AITask<CDoDSBot>* pastTask) override;
	TaskResult<CDoDSBot> OnTaskUpdate(CDoDSBot* bot) override;
	void OnTaskEnd(CDoDSBot* bot, AITask<CDoDSBot>* nextTask) override;
	bool IsBehaviorRunning(int id, int flags, bool ismod) override;

	const char* GetName() const override { return "DeployBomb"; }
private:
	CHandle<CBaseEntity> m_target;
	CMeshNavigator m_nav;
	bool m_isObjective; // if true, we are bombing a control point, otherwise it's a map obstacle
};


#endif // !__NAVBOT_DODSBOT_DEPLOY_BOMB_TASK_H_
