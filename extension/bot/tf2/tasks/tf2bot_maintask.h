#ifndef NAVBOT_TF2BOT_MAIN_TASK_H_
#define NAVBOT_TF2BOT_MAIN_TASK_H_
#pragma once

#include <sdkports/sdk_timers.h>

#ifdef EXT_DEBUG
#include <bot/interfaces/path/meshnavigator.h>
#endif // EXT_DEBUG


class CTF2Bot;
class WeaponInfo;
class CTF2BotSensor;


class CTF2BotMainTask : public AITask<CTF2Bot>
{
public:
	CTF2BotMainTask()
	{
	}

	AITask<CTF2Bot>* InitialNextTask(CTF2Bot* bot) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;
	
	TaskEventResponseResult<CTF2Bot> OnTestEventPropagation(CTF2Bot* bot) override;

	std::shared_ptr<const CKnownEntity> SelectTargetThreat(CBaseBot* me, std::shared_ptr<const CKnownEntity> threat1, std::shared_ptr<const CKnownEntity> threat2) override;
	Vector GetTargetAimPos(CBaseBot* me, edict_t* entity, CBaseExtPlayer* player = nullptr) override;

	TaskEventResponseResult<CTF2Bot> OnKilled(CTF2Bot* bot, const CTakeDamageInfo& info) override;

	const char* GetName() const override { return "MainTask"; }

private:
	CountdownTimer m_weaponswitchtimer;

	bool AllowedToSwitchWeapon();
	void FireWeaponAtEnemy(CTF2Bot* me, const CKnownEntity* threat);
	void UpdateLook(CTF2Bot* me, const CKnownEntity* threat);
	void InternalAimAtEnemyPlayer(CTF2Bot* me, CBaseExtPlayer* player, Vector& result);
	void InternalAimWithRocketLauncher(CTF2Bot* me, CBaseExtPlayer* player, Vector& result, const WeaponInfo* info, CTF2BotSensor* sensor);
	void InternalAimWithBallisticWeapon(CTF2Bot* me, CBaseExtPlayer* player, Vector& result, const WeaponInfo* info, CTF2BotSensor* sensor);
	std::shared_ptr<const CKnownEntity> InternalSelectTargetThreat(CTF2Bot* me, std::shared_ptr<const CKnownEntity> threat1, std::shared_ptr<const CKnownEntity> threat2);
};

inline bool CTF2BotMainTask::AllowedToSwitchWeapon()
{
	if (!m_weaponswitchtimer.HasStarted() || m_weaponswitchtimer.IsElapsed())
		return true;

	return false;
}

#ifdef EXT_DEBUG

class CTF2BotDevTask : public AITask<CTF2Bot>
{
public:
	CTF2BotDevTask(const Vector& moveTo);

	virtual TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	virtual TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	virtual TaskEventResponseResult<CTF2Bot> OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason) override;
	virtual TaskEventResponseResult<CTF2Bot> OnMoveToSuccess(CTF2Bot* bot, CPath* path) override;

	virtual const char* GetName() const override { return "DebugMoveToOrigin"; }
private:
	Vector m_goal;
	CountdownTimer m_repathtimer;
	CMeshNavigator m_nav;
};

#endif // EXT_DEBUG

#endif // !NAVBOT_TF2BOT_MAIN_TASK_H_
