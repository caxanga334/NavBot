#ifndef __NAVBOT_CSS_BOT_DEFEND_PLANTED_BOMB_TASK_H_
#define __NAVBOT_CSS_BOT_DEFEND_PLANTED_BOMB_TASK_H_

class CCSSBotDefendPlantedBombTask : public AITask<CCSSBot>
{
public:
	AITask<CCSSBot>* InitialNextTask(CCSSBot* bot) override;
	TaskResult<CCSSBot> OnTaskUpdate(CCSSBot* bot) override;

	const char* GetName() const override { return "DefendPlantedBomb"; }
};

#endif // !__NAVBOT_CSS_BOT_DEFEND_PLANTED_BOMB_TASK_H_