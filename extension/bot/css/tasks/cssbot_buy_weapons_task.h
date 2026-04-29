#ifndef __NAVBOT_CSS_BOT_BUY_WEAPONS_TASK_H_
#define __NAVBOT_CSS_BOT_BUY_WEAPONS_TASK_H_

/**
 * @brief This task makes the bots buy weapons and wait for the freeze time to be over.
 */
class CCSSBotBuyWeaponsTask : public AITask<CCSSBot>
{
public:
	TaskResult<CCSSBot> OnTaskStart(CCSSBot* bot, AITask<CCSSBot>* pastTask) override;
	TaskResult<CCSSBot> OnTaskUpdate(CCSSBot* bot) override;

	const char* GetName() const override { return "BuyWeapons"; }
private:
	CountdownTimer m_buydelay;
	bool m_didbuy;
};


#endif // !__NAVBOT_CSS_BOT_BUY_WEAPONS_TASK_H_
