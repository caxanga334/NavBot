#ifndef __NAVBOT_TF2_BOT_USE_TELEPORTER_TASK_H_
#define __NAVBOT_TF2_BOT_USE_TELEPORTER_TASK_H_

class CTF2BotUseTeleporterTask : public AITask<CTF2Bot>
{
public:
	CTF2BotUseTeleporterTask(CBaseEntity* teleporter);

	static bool IsPossible(CTF2Bot* bot, CBaseEntity** teleporter);

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	const char* GetName() const override { return "UseTeleporter"; }
private:
	CHandle<CBaseEntity> m_teleporter;
	CMeshNavigator m_nav;
	CountdownTimer m_timeout;
};

#endif // !__NAVBOT_TF2_BOT_USE_TELEPORTER_TASK_H_
