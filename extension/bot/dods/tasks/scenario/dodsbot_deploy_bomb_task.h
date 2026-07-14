#ifndef __NAVBOT_DODSBOT_DEPLOY_BOMB_TASK_H_
#define __NAVBOT_DODSBOT_DEPLOY_BOMB_TASK_H_

class CDoDSBotDeployBombTask : public AITask<CDoDSBot>
{
public:
	CDoDSBotDeployBombTask(CBaseEntity* target, const bool isObjective = true);

	TaskResult<CDoDSBot> OnTaskStart(CDoDSBot* bot, AITask<CDoDSBot>* pastTask) override;
	TaskResult<CDoDSBot> OnTaskUpdate(CDoDSBot* bot) override;
	void OnTaskEnd(CDoDSBot* bot, AITask<CDoDSBot>* nextTask) override;

	// Eat these events
	TaskEventResponseResult<CDoDSBot> OnBombPlanted(CDoDSBot* bot, const Vector& position, const int teamIndex, CBaseEntity* player, CBaseEntity* ent) override
	{
		return TryToMaintain(PRIORITY_LOW);
	}
	TaskEventResponseResult<CDoDSBot> OnBombDefused(CDoDSBot* bot, const Vector& position, const int teamIndex, CBaseEntity* player, CBaseEntity* ent) override
	{
		return TryToMaintain(PRIORITY_LOW);
	}
	TaskEventResponseResult<CDoDSBot> OnPathStatusChanged(CDoDSBot* bot) override { return TryToMaintain(PRIORITY_LOW); }

	const char* GetName() const override { return "DeployBomb"; }
private:
	CHandle<CBaseEntity> m_target;
	CMeshNavigator m_nav;
	bool m_isObjective; // if true, we are bombing a control point, otherwise it's a map obstacle
};


#endif // !__NAVBOT_DODSBOT_DEPLOY_BOMB_TASK_H_
