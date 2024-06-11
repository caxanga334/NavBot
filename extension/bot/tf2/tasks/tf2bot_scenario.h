#ifndef NAVBOT_TF2BOT_SCENARIO_TASK_H_
#define NAVBOT_TF2BOT_SCENARIO_TASK_H_
#pragma once

class CTF2Bot;

class CTF2BotScenarioTask : public AITask<CTF2Bot>
{
public:
	AITask<CTF2Bot>* InitialNextTask(CTF2Bot* bot) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	const char* GetName() const override { return "Scenario"; }
};

#endif // !NAVBOT_TF2BOT_SCENARIO_TASK_H_
