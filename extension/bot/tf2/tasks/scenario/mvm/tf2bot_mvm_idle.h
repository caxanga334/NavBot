#ifndef NAVBOT_TF2BOT_TASK_MVM_IDLE_H_
#define NAVBOT_TF2BOT_TASK_MVM_IDLE_H_
#pragma once

#include <bot/interfaces/path/meshnavigator.h>


class CTF2Bot;

class CTF2BotMvMIdleTask : public AITask<CTF2Bot>
{
public:
	CTF2BotMvMIdleTask()
	{
		m_goal.Init();
		m_isready = false;
	}

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	TaskEventResponseResult<CTF2Bot> OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason) override;
	TaskEventResponseResult<CTF2Bot> OnMoveToSuccess(CTF2Bot* bot, CPath* path) override;

	const char* GetName() const override { return "MvMIdle"; }
private:
	CMeshNavigator m_nav;
	Vector m_goal;
	CountdownTimer m_repathtimer;
	bool m_isready; // true if the bot is ready and should ready up when possible
};

#endif // !NAVBOT_TF2BOT_TASK_MVM_IDLE_H_
