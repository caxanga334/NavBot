#ifndef __NAVBOT_CSS_BOT_GUARD_BOMBSITE_TASK_H_
#define __NAVBOT_CSS_BOT_GUARD_BOMBSITE_TASK_H_

class CCSSBotGuardBombSiteTask : public AITask<CCSSBot>
{
public:
	AITask<CCSSBot>* InitialNextTask(CCSSBot* bot) override;
	TaskResult<CCSSBot> OnTaskUpdate(CCSSBot* bot) override;

	TaskEventResponseResult<CCSSBot> OnBombPlanted(CCSSBot* bot, const Vector& position, const int teamIndex, CBaseEntity* player, CBaseEntity* ent) override;

	const char* GetName() const override { return "GuardBombSite"; }
};

#endif // !__NAVBOT_CSS_BOT_GUARD_BOMBSITE_TASK_H_
