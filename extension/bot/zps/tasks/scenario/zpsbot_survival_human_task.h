#ifndef __NAVBOT_ZPSBOT_SURVIVAL_HUMAN_TASK_H_
#define __NAVBOT_ZPSBOT_SURVIVAL_HUMAN_TASK_H_

class CZPSBotSurvivalHumanTask : public AITask<CZPSBot>
{
public:

	TaskResult<CZPSBot> OnTaskUpdate(CZPSBot* bot) override;

	const char* GetName() const override { return "SurvivalHuman"; }

private:

};

#endif // !__NAVBOT_ZPSBOT_SURVIVAL_HUMAN_TASK_H_
