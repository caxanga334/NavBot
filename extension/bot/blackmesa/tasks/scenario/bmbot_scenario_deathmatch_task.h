#ifndef NAVBOT_BLACK_MESA_BOT_SCENARIO_DEATHMATCH_TASK_H_
#define NAVBOT_BLACK_MESA_BOT_SCENARIO_DEATHMATCH_TASK_H_

#include <sdkports/sdk_timers.h>

class CBlackMesaBot;

class CBlackMesaBotScenarioDeathmatchTask : public AITask<CBlackMesaBot>
{
public:
	TaskResult<CBlackMesaBot> OnTaskUpdate(CBlackMesaBot* bot) override;

	const char* GetName() const override { return "DeathmatchScenario"; }
private:


};

#endif // !NAVBOT_BLACK_MESA_BOT_SCENARIO_DEATHMATCH_TASK_H_
