#ifndef NAVBOT_TF2BOT_MAIN_TASK_H_
#define NAVBOT_TF2BOT_MAIN_TASK_H_
#pragma once

#include <sdkports/sdk_timers.h>

class CTF2Bot;
class WeaponInfo;
class CTF2BotSensor;

class CTF2BotMainTask : public AITask<CTF2Bot>
{
public:
	virtual AITask<CTF2Bot>* InitialNextTask() override;
	virtual TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;
	
	virtual CKnownEntity* SelectTargetThreat(CBaseBot* me, CKnownEntity* threat1, CKnownEntity* threat2) override;
	virtual Vector GetTargetAimPos(CBaseBot* me, edict_t* entity, CBaseExtPlayer* player = nullptr) override;


	virtual const char* GetName() const override { return "MainTask"; }

private:
	CountdownTimer m_weaponswitchtimer;

	bool AllowedToSwitchWeapon();
	void FireWeaponAtEnemy(CTF2Bot* me, CKnownEntity* threat);
	void SelectBestWeaponForEnemy(CTF2Bot* me, CKnownEntity* threat);
	void UpdateLook(CTF2Bot* me, CKnownEntity* threat);
	void InternalAimAtEnemyPlayer(CTF2Bot* me, CBaseExtPlayer* player, Vector& result);
	void InternalAimWithRocketLauncher(CTF2Bot* me, CBaseExtPlayer* player, Vector& result, const WeaponInfo* info, CTF2BotSensor* sensor);
	CKnownEntity* InternalSelectTargetThreat(CTF2Bot* me, CKnownEntity* threat1, CKnownEntity* threat2);
};

inline bool CTF2BotMainTask::AllowedToSwitchWeapon()
{
	if (!m_weaponswitchtimer.HasStarted())
		return true;

	if (m_weaponswitchtimer.IsElapsed())
		return true;

	return false;
}

#endif // !NAVBOT_TF2BOT_MAIN_TASK_H_
