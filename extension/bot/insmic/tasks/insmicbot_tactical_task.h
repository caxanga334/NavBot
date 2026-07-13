#ifndef __NAVBOT_INSMICBOT_TACTICAL_TASK_H_
#define __NAVBOT_INSMICBOT_TACTICAL_TASK_H_

class CInsMICBotTacticalTask : public AITask<CInsMICBot>
{
public:
	CInsMICBotTacticalTask();

	AITask<CInsMICBot>* InitialNextTask(CInsMICBot* bot) override;
	TaskResult<CInsMICBot> OnTaskUpdate(CInsMICBot* bot) override;

	TaskEventResponseResult<CInsMICBot> OnNavAreaChanged(CInsMICBot* bot, CNavArea* oldArea, CNavArea* newArea) override;
	TaskEventResponseResult<CInsMICBot> OnPluginCommand(CInsMICBot* bot, IEventListener::PluginCommandTypes type, const IEventListener::PluginCommandData& data) override;

	const char* GetName() const override { return "Tactical"; }

private:

};

#endif // !__NAVBOT_INSMICBOT_TACTICAL_TASK_H_
