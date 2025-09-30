#ifndef __NAVBOT_DODSBOT_ATTACK_CONTROL_POINT_TASK_H_
#define __NAVBOT_DODSBOT_ATTACK_CONTROL_POINT_TASK_H_

class CDoDSBotAttackControlPointTask : public AITask<CDoDSBot>
{
public:
	CDoDSBotAttackControlPointTask(const CDayOfDefeatSourceMod::DoDControlPoint* controlpoint);
	static bool IsPossible(CDoDSBot* bot, const CDayOfDefeatSourceMod::DoDControlPoint** controlpoint);

	AITask<CDoDSBot>* InitialNextTask(CDoDSBot* bot) override;

	TaskResult<CDoDSBot> OnTaskStart(CDoDSBot* bot, AITask<CDoDSBot>* pastTask) override;
	TaskResult<CDoDSBot> OnTaskUpdate(CDoDSBot* bot) override;

	const char* GetName() const override { return "AttackControlPoint"; }
private:
	const CDayOfDefeatSourceMod::DoDControlPoint* m_controlpoint;
};

#endif // !__NAVBOT_DODSBOT_ATTACK_CONTROL_POINT_TASK_H_
