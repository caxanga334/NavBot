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
	CBlackMesaBotFindAmmoTask(blackmesa::BMAmmoIndex ammoType);

	static bool IsPossible(CBlackMesaBot* bot, blackmesa::BMAmmoIndex& ammoInNeed);

	TaskResult<CBlackMesaBot> OnTaskStart(CBlackMesaBot* bot, AITask<CBlackMesaBot>* pastTask) override;
	TaskResult<CBlackMesaBot> OnTaskUpdate(CBlackMesaBot* bot) override;

	const char* GetName() const override { return "FindAmmo"; }
private:
	blackmesa::BMAmmoIndex m_ammoIndex;
	int m_maxCarry;
	std::string m_classname;
	CHandle<CBaseEntity> m_item;
	CountdownTimer m_repathtimer;
	CMeshNavigator m_nav;
	Vector m_goal;

	void SetGoalEntity(CBaseEntity* goalEnt);
	static bool IsItemValid(CBaseEntity* item);
	static bool FindAmmoToPickup(CBlackMesaBot* bot, CBaseEntity** ammo, const std::string& classname, const float maxRange = 2048.0f, const bool selectNearest = false);
};

#endif // !NAVBOT_BLACK_MESA_BOT_FIND_AMMO_TASK_H_
