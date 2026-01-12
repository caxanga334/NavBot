#ifndef __NAVBOT_DODSBOT_DEPLOY_MG_TASK_H_
#define __NAVBOT_DODSBOT_DEPLOY_MG_TASK_H_

class CDoDSBotDeployMachineGunTask : public AITask<CDoDSBot>
{
public:
	CDoDSBotDeployMachineGunTask();
	static bool IsPossible(CDoDSBot* bot);
	TaskResult<CDoDSBot> OnTaskStart(CDoDSBot* bot, AITask<CDoDSBot>* pastTask) override;
	TaskResult<CDoDSBot> OnTaskUpdate(CDoDSBot* bot) override;
	void OnTaskEnd(CDoDSBot* bot, AITask<CDoDSBot>* nextTask) override;

	TaskEventResponseResult<CDoDSBot> OnMoveToSuccess(CDoDSBot* bot, CPath* path) override;

	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override;
	QueryAnswerType ShouldAssistTeammate(CBaseBot* me, CBaseEntity* teammate) override;
	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override;

	const char* GetName() const override { return "DeployMG"; }
private:
	CMeshNavigator m_nav;
	CDoDSWaypoint* m_waypoint;
	Vector m_pos;
	QAngle m_deployAngle;
	CountdownTimer m_campTimer;
	CountdownTimer m_deployingTimer;
	bool m_atPos;
};


#endif // __NAVBOT_DODSBOT_DEPLOY_MG_TASK_H_