#ifndef NAVBOT_TF2BOT_TASKS_ENGINEER_REPAIR_OBJECT_H_
#define NAVBOT_TF2BOT_TASKS_ENGINEER_REPAIR_OBJECT_H_

#include <sdkports/sdk_ehandle.h>
#include <bot/interfaces/path/meshnavigator.h>

class CTF2Bot;

class CTF2BotEngineerRepairObjectTask : public AITask<CTF2Bot>
{
public:
	CTF2BotEngineerRepairObjectTask(CBaseEntity* object);

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	QueryAnswerType ShouldSeekAndDestroy(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_NO; }
	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override;

	const char* GetName() const override { return "RepairObject"; }

private:
	CHandle<CBaseEntity> m_object;
	CMeshNavigator m_nav;
	bool m_sentry;

	static constexpr auto get_object_melee_range() { return 64.0f; }
};

#endif // !NAVBOT_TF2BOT_TASKS_ENGINEER_REPAIR_OBJECT_H_
