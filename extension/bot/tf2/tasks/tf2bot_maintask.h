#ifndef NAVBOT_TF2BOT_MAIN_TASK_H_
#define NAVBOT_TF2BOT_MAIN_TASK_H_
#pragma once

class CTF2Bot;
class WeaponInfo;
class CTF2BotSensor;

class CTF2BotMainTask : public AITask<CTF2Bot>
{
public:
	virtual AITask<CTF2Bot>* InitialNextTask() override;
	virtual TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;
	
	virtual Vector GetTargetAimPos(CBaseBot* me, edict_t* entity, CBaseExtPlayer* player = nullptr) override;


	virtual const char* GetName() const override { return "MainTask"; }

private:
	void InternalAimAtEnemyPlayer(CTF2Bot* me, CBaseExtPlayer* player, Vector& result);
	void InternalAimWithRocketLauncher(CTF2Bot* me, CBaseExtPlayer* player, Vector& result, const WeaponInfo* info, CTF2BotSensor* sensor);
};

#endif // !NAVBOT_TF2BOT_MAIN_TASK_H_
