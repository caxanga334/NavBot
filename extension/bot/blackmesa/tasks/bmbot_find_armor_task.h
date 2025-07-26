#ifndef NAVBOT_BLACK_MESA_BOT_FIND_ARMOR_TASK_H_
#define NAVBOT_BLACK_MESA_BOT_FIND_ARMOR_TASK_H_

#include <sdkports/sdk_ehandle.h>
#include <bot/interfaces/path/meshnavigator.h>

class CBlackMesaBot;

class CBlackMesaBotFindArmorTask : public AITask<CBlackMesaBot>
{
public:

	static bool IsPossible(CBlackMesaBot* bot);

	TaskResult<CBlackMesaBot> OnTaskStart(CBlackMesaBot* bot, AITask<CBlackMesaBot>* pastTask) override;
	TaskResult<CBlackMesaBot> OnTaskUpdate(CBlackMesaBot* bot) override;

	TaskEventResponseResult<CBlackMesaBot> OnMoveToSuccess(CBlackMesaBot* bot, CPath* path) override;

	const char* GetName() const override { return "FindArmor"; }
private:
	CHandle<CBaseEntity> m_armorSource;
	CountdownTimer m_timeout;
	CMeshNavigator m_nav;
	Vector m_goal;
	bool m_isCharger;

	void SetArmorSource(CBaseEntity* source, CBlackMesaBot* bot);
	bool IsArmorSourceValid();

	static bool FindArmorSource(CBlackMesaBot* bot, CBaseEntity** armorSource, const float maxRange = 2048.0f, const bool filterByDistance = false);
};

#endif // !NAVBOT_BLACK_MESA_BOT_FIND_ARMOR_TASK_H_
