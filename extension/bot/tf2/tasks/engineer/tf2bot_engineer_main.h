#ifndef NAVBOT_TF2BOT_TASKS_ENGINEER_MAIN_H_
#define NAVBOT_TF2BOT_TASKS_ENGINEER_MAIN_H_

#include <sdkports/sdk_timers.h>

class CTF2Bot;

/**
 * @brief Primary engineer behavior
 */
class CTF2BotEngineerMainTask : public AITask<CTF2Bot>
{
public:
	CTF2BotEngineerMainTask();

	AITask<CTF2Bot>* InitialNextTask(CTF2Bot* bot) override;

	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	const char* GetName() const override { return "EngineerMain"; }

	static constexpr auto HELP_ALLY_BUILDING_MAX_RANGE_SQR = 1024.f * 1024.0f;
private:
	CountdownTimer m_friendlyBuildingScan;

	CBaseEntity* ScanAllyBuildings(CTF2Bot* me, bool &is_upgrade);
};

#endif // !NAVBOT_TF2BOT_TASKS_ENGINEER_MAIN_H_
