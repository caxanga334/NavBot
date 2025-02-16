#ifndef NAVBOT_BOT_SHARED_PREREQUISITE_WAIT_TASK_H_
#define NAVBOT_BOT_SHARED_PREREQUISITE_WAIT_TASK_H_

#include <extension.h>
#include <bot/basebot.h>
#include <sdkports/sdk_timers.h>

template <typename BT>
class CBotSharedPrereqWaitTask : public AITask<BT>
{
public:
	CBotSharedPrereqWaitTask(float waitTime) :
		AITask<BT>()
	{
		m_waitTime = waitTime;
	}

	TaskResult<BT> OnTaskStart(BT* bot, AITask<BT>* pastTask) override;
	TaskResult<BT> OnTaskUpdate(BT* bot) override;

	const char* GetName() const override { return "Wait"; }

private:
	float m_waitTime;
	CountdownTimer m_Timer;
};

template<typename BT>
inline TaskResult<BT> CBotSharedPrereqWaitTask<BT>::OnTaskStart(BT* bot, AITask<BT>* pastTask)
{
	m_Timer.Start(m_waitTime);

	return AITask<BT>::Continue();
}

template<typename BT>
inline TaskResult<BT> CBotSharedPrereqWaitTask<BT>::OnTaskUpdate(BT* bot)
{
	if (m_Timer.IsElapsed())
	{
		return AITask<BT>::Done("Timer elapsed!");
	}

	return AITask<BT>::Continue();
}

#endif // !NAVBOT_BOT_SHARED_PREREQUISITE_WAIT_TASK_H_