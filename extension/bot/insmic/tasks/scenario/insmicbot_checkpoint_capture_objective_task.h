#ifndef __NAVBOT_INSMICBOT_CHECKPOINT_CAPTURE_OBJECTIVE_TASK_H_
#define __NAVBOT_INSMICBOT_CHECKPOINT_CAPTURE_OBJECTIVE_TASK_H_

class CInsMICBotCheckPointCaptureObjectiveTask : public AITask<CInsMICBot>
{
public:
	CInsMICBotCheckPointCaptureObjectiveTask(CBaseEntity* objective);
	TaskResult<CInsMICBot> OnTaskStart(CInsMICBot* bot, AITask<CInsMICBot>* pastTask) override;
	TaskResult<CInsMICBot> OnTaskUpdate(CInsMICBot* bot) override;

	const char* GetName() const override { return "CaptureObjective"; }
private:
	CMeshNavigator m_nav;
	Vector m_moveTo;
	CHandle<CBaseEntity> m_objective;
};

#endif // !__NAVBOT_INSMICBOT_CHECKPOINT_CAPTURE_OBJECTIVE_TASK_H_
