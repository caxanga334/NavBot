#ifndef __NAVBOT_INSMICBOT_CHECKPOINT_PATROL_OBJECTIVE_TASK_H_
#define __NAVBOT_INSMICBOT_CHECKPOINT_PATROL_OBJECTIVE_TASK_H_

class CInsMICBotCheckPointPatrolObjectiveTask : public AITask<CInsMICBot>
{
public:
	CInsMICBotCheckPointPatrolObjectiveTask(CBaseEntity* objective);
	TaskResult<CInsMICBot> OnTaskStart(CInsMICBot* bot, AITask<CInsMICBot>* pastTask) override;
	TaskResult<CInsMICBot> OnTaskUpdate(CInsMICBot* bot) override;

	TaskEventResponseResult<CInsMICBot> OnMoveToSuccess(CInsMICBot* bot, CPath* path) override;

	const char* GetName() const override { return "PatrolObjective"; }
private:
	CMeshNavigator m_nav;
	Vector m_moveTo;
	CHandle<CBaseEntity> m_objective;
};

#endif // !__NAVBOT_INSMICBOT_CHECKPOINT_PATROL_OBJECTIVE_TASK_H_
