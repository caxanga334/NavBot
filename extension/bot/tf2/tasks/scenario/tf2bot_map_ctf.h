#ifndef NAVBOT_TF2BOT_MAP_CTF_TASK_H_
#define NAVBOT_TF2BOT_MAP_CTF_TASK_H_
#pragma once

#include <bot/interfaces/path/meshnavigator.h>

class CTF2Bot;
struct edict_t;

class CTF2BotCTFMonitorTask : public AITask<CTF2Bot>
{
public:
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	const char* GetName() const override { return "CTFMonitor"; }
};

class CTF2BotCTFFetchFlagTask : public AITask<CTF2Bot>
{
public:
	CTF2BotCTFFetchFlagTask(edict_t* flag);

	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	TaskEventResponseResult<CTF2Bot> OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason) override;

	const char* GetName() const override { return "FetchFlag"; }
private:
	CMeshNavigator m_nav;
	CBaseHandle m_flag;
	Vector m_goalpos;
	CountdownTimer m_repathtimer;
};

class CTF2BotCTFDeliverFlagTask : public AITask<CTF2Bot>
{
public:
	TaskResult<CTF2Bot> OnTaskStart(CTF2Bot* bot, AITask<CTF2Bot>* pastTask) override;
	TaskResult<CTF2Bot> OnTaskUpdate(CTF2Bot* bot) override;

	TaskEventResponseResult<CTF2Bot> OnMoveToFailure(CTF2Bot* bot, CPath* path, IEventListener::MovementFailureType reason) override;

	QueryAnswerType ShouldHurry(CBaseBot* me) override { return ANSWER_YES; }
	QueryAnswerType ShouldRetreat(CBaseBot* me) override { return ANSWER_NO; }

	const char* GetName() const override { return "DeliverFlag"; }
private:
	CMeshNavigator m_nav;
	Vector m_goalpos;
	CountdownTimer m_repathtimer;
};

#endif // !NAVBOT_TF2BOT_MAP_CTF_TASK_H_
