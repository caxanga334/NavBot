#ifndef __NAVBOT_CSS_BOT_TACTICAL_TASK_H_
#define __NAVBOT_CSS_BOT_TACTICAL_TASK_H_

class CCSSBotTacticalTask : public AITask<CCSSBot>
{
public:
	AITask<CCSSBot>* InitialNextTask(CCSSBot* bot) override;
	TaskResult<CCSSBot> OnTaskStart(CCSSBot* bot, AITask<CCSSBot>* pastTask) override;
	TaskResult<CCSSBot> OnTaskUpdate(CCSSBot* bot) override;
	TaskEventResponseResult<CCSSBot> OnKilled(CCSSBot* bot, const CTakeDamageInfo& info) override;
	TaskEventResponseResult<CCSSBot> OnNavAreaChanged(CCSSBot* bot, CNavArea* oldArea, CNavArea* newArea) override;
	TaskEventResponseResult<CCSSBot> OnDangerousEntityChanged(CCSSBot* bot, CBaseEntity* newent, CBaseEntity* oldent) override;
	TaskEventResponseResult<CCSSBot> OnPluginCommand(CCSSBot* bot, IEventListener::PluginCommandTypes type, const IEventListener::PluginCommandData& data) override;
	QueryAnswerType ShouldHurry(CBaseBot* me) override;

	const char* GetName() const override { return "Tactical"; }
private:
	CountdownTimer m_gunEquipTimer;
};


#endif // !__NAVBOT_CSS_BOT_TACTICAL_TASK_H_
