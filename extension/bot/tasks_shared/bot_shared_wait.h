#ifndef __NAVBOT_BOT_SHARED_WAIT_TASK_H_
#define __NAVBOT_BOT_SHARED_WAIT_TASK_H_

#include <extension.h>
#include <util/helpers.h>
#include <bot/basebot.h>
#include <mods/modhelpers.h>

/**
 * @brief Generic simple task to make the bot wait N seconds.
 * @tparam BT Bot class
 */
template <typename BT>
class CBotSharedWaitTask : public AITask<BT>
{
public:
	/**
	 * @brief Constructor.
	 * @param waitTime Time the bot should wait in seconds
	 */
	CBotSharedWaitTask(float waitTime)
	{
		m_duration = waitTime;
	}

	TaskResult<BT> OnTaskStart(BT* bot, AITask<BT>* pastTask) override
	{
		m_timer.Start(m_duration);
		return AITask<BT>::Continue();
	}


	TaskResult<BT> OnTaskUpdate(BT* bot) override
	{
		if (m_timer.IsElapsed())
		{
			return AITask<BT>::Done("Timer elapsed!");
		}

		return AITask<BT>::Continue();
	}

	const char* GetName() const override { return "Wait"; }

private:
	CountdownTimer m_timer;
	float m_duration;
};

template <typename BT>
class CBotSharedWaitForSquadMembersTask : public AITask<BT>
{
public:
	/**
	 * @brief Constructor.
	 * @param timeout Maximum amount of time to wait.
	 * @param minDistance Minimum distace between the leader and squad members.
	 */
	CBotSharedWaitForSquadMembersTask(float timeout, float minDistance)
	{
		m_duration = timeout;
		m_minDistance = minDistance;
	}

	TaskResult<BT> OnTaskStart(BT* bot, AITask<BT>* pastTask) override
	{
		m_timer.Start(m_duration);
		return AITask<BT>::Continue();
	}

	TaskResult<BT> OnTaskUpdate(BT* bot) override
	{
		if (m_timer.IsElapsed())
		{
			return AITask<BT>::Done("Timer elapsed!");
		}

		ISquad* squad = bot->GetSquadInterface();

		if (!squad->IsSquadValid())
		{
			return AITask<BT>::Done("No longer in an squad.");
		}

		if (!squad->IsSquadLeader())
		{
			return AITask<BT>::Done("Must be a squad leader!");
		}

		if (squad->IsHumanLedSquad())
		{
			return AITask<BT>::Done("Leader is a human!");
		}

		std::size_t numClose = 0;

		const ISquad::SquadData* data = squad->GetSquadData();
		Vector pos = bot->GetAbsOrigin();
		float min_dist = m_minDistance;
		auto func = [&numClose, &pos, min_dist](const ISquad::Member& member) {
			if ((pos - member.GetPosition()).IsLengthLessThan(min_dist))
			{
				numClose++;
			}
		};

		data->ForEachMember(func);

		if (numClose >= (data->GetMemberCount() - 1U))
		{
			return AITask<BT>::Done("All squad members are near me!");
		}

		return AITask<BT>::Continue();
	}

	const char* GetName() const override { return "WaitForSquadMembers"; }

private:
	CountdownTimer m_timer;
	float m_duration;
	float m_minDistance;
};

#endif // !__NAVBOT_BOT_SHARED_WAIT_TASK_H_
