#ifndef NAVBOT_BLACK_MESA_BOT_FIND_AMMO_TASK_H_
#define NAVBOT_BLACK_MESA_BOT_FIND_AMMO_TASK_H_

#include <string>
#include <sdkports/sdk_ehandle.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <mods/blackmesa/blackmesa_shareddefs.h>

class CBlackMesaBot;

class CBlackMesaBotFindAmmoTask : public AITask<CBlackMesaBot>
{
public:
	CBlackMesaBotFindAmmoTask(CBaseEntity* ammo, blackmesa::BMAmmoIndex ammoIndex);

	static bool IsPossible(CBlackMesaBot* bot, CBaseEntity** ammo, blackmesa::BMAmmoIndex& ammoIndex);

	TaskResult<CBlackMesaBot> OnTaskStart(CBlackMesaBot* bot, AITask<CBlackMesaBot>* pastTask) override;
	TaskResult<CBlackMesaBot> OnTaskUpdate(CBlackMesaBot* bot) override;

	const char* GetName() const override { return "FindAmmo"; }
private:
	int m_maxCarry;
	blackmesa::BMAmmoIndex m_ammoIndex;
	CHandle<CBaseEntity> m_item;
	CMeshNavigator m_nav;
	Vector m_goal;

	static bool IsItemValid(CBaseEntity* item);
};

#endif // !NAVBOT_BLACK_MESA_BOT_FIND_AMMO_TASK_H_
