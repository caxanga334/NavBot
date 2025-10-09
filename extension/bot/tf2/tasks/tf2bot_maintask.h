#ifndef NAVBOT_TF2BOT_MAIN_TASK_H_
#define NAVBOT_TF2BOT_MAIN_TASK_H_
#pragma once

#include <sdkports/sdk_timers.h>

class CTF2Bot;
class CTF2BotWeapon;
class CKnownEntity;
class CTF2BotSensor;
class WeaponAttackFunctionInfo;

class CTF2BotMainTask : public AITask<CTF2Bot>
{
public:
	CTF2BotMainTask()
	{
	}

	AITask<CTF2Bot>* InitialNextTask(CTF2Bot* bot) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;
	
	TaskEventResponseResult<CTF2Bot> OnDebugMoveToCommand(CTF2Bot* bot, const Vector& moveTo) override;

	const CKnownEntity* SelectTargetThreat(CBaseBot* me, const CKnownEntity* threat1, const CKnownEntity* threat2) override;
	Vector GetTargetAimPos(CBaseBot* me, CBaseEntity* entity, DesiredAimSpot desiredAim = AIMSPOT_NONE) override;

	TaskEventResponseResult<CTF2Bot> OnKilled(CTF2Bot* bot, const CTakeDamageInfo& info) override;

	const char* GetName() const override { return "MainTask"; }

private:
	const CKnownEntity* InternalSelectTargetThreat(CTF2Bot* me, const CKnownEntity* threat1, const CKnownEntity* threat2);
	bool IsImmediateThreat(CTF2Bot* me, const CKnownEntity* threat) const;
};

#endif // !NAVBOT_TF2BOT_MAIN_TASK_H_
