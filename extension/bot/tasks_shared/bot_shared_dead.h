#ifndef __NAVBOT_BOT_SHARED_DEAD_TASK_H_
#define __NAVBOT_BOT_SHARED_DEAD_TASK_H_

#include <utility>
#include <extension.h>
#include <bot/basebot.h>

/**
 * @brief General purpose task for when a bot is killed.
 * @tparam BT Bot class.
 * @tparam ExitTask Class of the task to go back if the bot is alive.
 */
template <typename BT, typename ExitTask>
class CBotSharedDeadTask : public AITask<BT>
{
public:
	template <typename... CArgs>
	CBotSharedDeadTask(CArgs&&... _args)
	{
		m_exitTask = new ExitTask(std::forward<CArgs>(_args)...);
	}

	~CBotSharedDeadTask() override
	{
		if (m_exitTask)
		{
			delete m_exitTask;
			m_exitTask = nullptr;
		}
	}

	TaskResult<BT> OnTaskUpdate(BT* bot) override;

	const char* GetName() const override { return "Dead"; }
private:
	ExitTask* m_exitTask;
};

template<typename BT, typename ExitTask>
inline TaskResult<BT> CBotSharedDeadTask<BT, ExitTask>::OnTaskUpdate(BT* bot)
{
	if (bot->IsAlive())
	{
		ExitTask* task = m_exitTask; // copy task
		m_exitTask = nullptr; // set to NULL so the destructor doesn't delete it
		return AITask<BT>::SwitchTo(task, "Bot is alive, switching tasks!");
	}

	return AITask<BT>::Continue();
}

#endif // !__NAVBOT_BOT_SHARED_DEAD_TASK_H_
