#ifndef NAVBOT_TF2BOT_TASK_MVM_UPGRADE_H_
#define NAVBOT_TF2BOT_TASK_MVM_UPGRADE_H_
#pragma once

#include <bot/interfaces/path/meshnavigator.h>
#include <basehandle.h>

class CTF2Bot;

class CTF2BotMvMUpgradeTask : public AITask<CTF2Bot>
{
public:
	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	TaskEventResponseResult<CTF2Bot> OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason) override;
	TaskEventResponseResult<CTF2Bot> OnMoveToSuccess(CTF2Bot* bot, CPath* path) override;

	const char* GetName() const override { return "MvMUpgrade"; }
private:
	CMeshNavigator m_nav;
	Vector m_goal;
	CountdownTimer m_buydelay;
	CBaseHandle m_upgradestation;

	bool SelectNearestUpgradeStation(CTF2Bot* me);
	void SetGoalPosition();
};

#endif // !NAVBOT_TF2BOT_TASK_MVM_UPGRADE_H_
