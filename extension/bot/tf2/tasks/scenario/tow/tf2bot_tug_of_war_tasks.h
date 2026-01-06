#ifndef __NAVBOT_TF2BOT_TUG_OF_WAR_TASKS_H_
#define __NAVBOT_TF2BOT_TUG_OF_WAR_TASKS_H_

// Main behavior for tug of war
class CTF2BotTugOfWarMonitorTask : public AITask<CTF2Bot>
{
public:
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	const char* GetName() const override { return "TugOfWar"; }
private:

};

// Attack the control point behavior
class CTF2BotTugOfWarAttackTask : public AITask<CTF2Bot>
{
public:
	CTF2BotTugOfWarAttackTask();
	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	const char* GetName() const override { return "TOWAttack"; }
private:
	CHandle<CBaseEntity> m_goal;
	CMeshNavigator m_nav;
};

#endif // !__NAVBOT_TF2BOT_TUG_OF_WAR_TASKS_H_
