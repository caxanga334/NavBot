#ifndef __NAVBOT_ZPSBOT_TACTICAL_TASK_H_
#define __NAVBOT_ZPSBOT_TACTICAL_TASK_H_

class CZPSBotTacticalTask : public AITask<CZPSBot>
{
public:

	AITask<CZPSBot>* InitialNextTask(CZPSBot* bot) override;
	TaskResult<CZPSBot> OnTaskUpdate(CZPSBot* bot) override;

	TaskEventResponseResult<CZPSBot> OnNavAreaChanged(CZPSBot* bot, CNavArea* oldArea, CNavArea* newArea) override;

	const char* GetName() const override { return "Tactical"; }

private:

};


#endif // !__NAVBOT_ZPSBOT_TACTICAL_TASK_H_
