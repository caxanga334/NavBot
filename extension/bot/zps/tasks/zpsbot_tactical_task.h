#ifndef __NAVBOT_ZPSBOT_TACTICAL_TASK_H_
#define __NAVBOT_ZPSBOT_TACTICAL_TASK_H_

class CZPSBotTacticalTask : public AITask<CZPSBot>
{
public:

	AITask<CZPSBot>* InitialNextTask(CZPSBot* bot) override;
	TaskResult<CZPSBot> OnTaskUpdate(CZPSBot* bot) override;

	TaskEventResponseResult<CZPSBot> OnInjured(CZPSBot* bot, const CTakeDamageInfo& info) override;
	TaskEventResponseResult<CZPSBot> OnNavAreaChanged(CZPSBot* bot, CNavArea* oldArea, CNavArea* newArea) override;
	TaskEventResponseResult<CZPSBot> OnSound(CZPSBot* bot, CBaseEntity* source, const Vector& position, SoundType type, const float maxRadius) override;
	TaskEventResponseResult<CZPSBot> OnPluginCommand(CZPSBot* bot, IEventListener::PluginCommandTypes type, const IEventListener::PluginCommandData& data) override;

	const char* GetName() const override { return "Tactical"; }

private:

};


#endif // !__NAVBOT_ZPSBOT_TACTICAL_TASK_H_
