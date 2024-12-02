#ifndef NAVBOT_TF2_DEATHMATCH_SCENARIO_TASK_H_
#define NAVBOT_TF2_DEATHMATCH_SCENARIO_TASK_H_

class CTF2BotDeathmatchScenarioTask : public AITask<CTF2Bot>
{
public:
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	const char* GetName() const override { return "Deathmatch"; }
};

#endif //!NAVBOT_TF2_DEATHMATCH_SCENARIO_TASK_H_