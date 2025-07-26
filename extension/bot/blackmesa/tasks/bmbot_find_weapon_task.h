#ifndef NAVBOT_BLACK_MESA_BOT_FIND_WEAPON_TASK_H_
#define NAVBOT_BLACK_MESA_BOT_FIND_WEAPON_TASK_H_

#include <sdkports/sdk_ehandle.h>
#include <bot/interfaces/path/meshnavigator.h>

class CBlackMesaBot;

class CBlackMesaBotFindWeaponTask : public AITask<CBlackMesaBot>
{
public:

	static bool IsPossible(CBlackMesaBot* bot);

	TaskResult<CBlackMesaBot> OnTaskStart(CBlackMesaBot* bot, AITask<CBlackMesaBot>* pastTask) override;
	TaskResult<CBlackMesaBot> OnTaskUpdate(CBlackMesaBot* bot) override;
	void OnTaskEnd(CBlackMesaBot* bot, AITask<CBlackMesaBot>* nextTask) override;

	TaskEventResponseResult<CBlackMesaBot> OnMoveToSuccess(CBlackMesaBot* bot, CPath* path) override;
	TaskEventResponseResult<CBlackMesaBot> OnWeaponEquip(CBlackMesaBot* bot, CBaseEntity* weapon) override;

	const char* GetName() const override { return "FindWeapon"; }
private:
	CHandle<CBaseEntity> m_weapon;
	CMeshNavigator m_nav;
	Vector m_goal;

	bool IsWeaponValid();

	static bool FindWeaponToPickup(CBlackMesaBot* bot, CBaseEntity** weapon, const float maxRange = 4096.0f);
};

#endif // !NAVBOT_BLACK_MESA_BOT_FIND_WEAPON_TASK_H_
