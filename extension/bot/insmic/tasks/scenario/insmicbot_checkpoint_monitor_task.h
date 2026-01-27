#ifndef __NAVBOT_INSMICBOT_CHECKPOINT_MONITOR_TASK_H_
#define __NAVBOT_INSMICBOT_CHECKPOINT_MONITOR_TASK_H_

class CInsMICBotCheckPointMonitorTask : public AITask<CInsMICBot>
{
public:

	// AITask<CInsMICBot>* InitialNextTask(CInsMICBot* bot) override;
	// TaskResult<CInsMICBot> OnTaskStart(CInsMICBot* bot, AITask<CInsMICBot>* pastTask) override;
	TaskResult<CInsMICBot> OnTaskUpdate(CInsMICBot* bot) override;

	const char* GetName() const override { return "Checkpoint"; }
};

#endif // !__NAVBOT_INSMICBOT_CHECKPOINT_MONITOR_TASK_H_
