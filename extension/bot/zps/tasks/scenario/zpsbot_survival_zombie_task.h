#ifndef __NAVBOT_ZPSBOT_SURVIVAL_ZOMBIE_TASK_H_
#define __NAVBOT_ZPSBOT_SURVIVAL_ZOMBIE_TASK_H_

class CZPSBotSurvivalZombieTask : public AITask<CZPSBot>
{
public:

	TaskResult<CZPSBot> OnTaskUpdate(CZPSBot* bot) override;

	const char* GetName() const override { return "SurvivalZombie"; }

private:

};

#endif // !__NAVBOT_ZPSBOT_SURVIVAL_ZOMBIE_TASK_H_
