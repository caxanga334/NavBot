#ifndef __NAVBOT_BOT_SHARED_CLEAR_LKP_TASK_H_
#define __NAVBOT_BOT_SHARED_CLEAR_LKP_TASK_H_

#include <extension.h>
#include <bot/basebot.h>
#include <bot/basebot_pathcost.h>
#include <bot/interfaces/path/chasenavigator.h>

template <typename BT, typename CT = CBaseBotPathCost>
class CBotSharedClearEnemyLKPTask : public AITask<BT>
{
public:
	CBotSharedClearEnemyLKPTask(BT* bot, const Vector& LKP) : m_pathcost(bot), m_LKP(LKP)
	{
	}

	TaskResult<BT> OnTaskStart(BT* bot, AITask<BT>* pastTask) override
	{
		if (!m_nav.ComputePathToPosition(bot, m_LKP, m_pathcost))
		{
			return AITask<BT>::Done("No path to last known position!");
		}

		return AITask<BT>::Continue();
	}

	TaskResult<BT> OnTaskUpdate(BT* bot) override
	{
		const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(false);

		if (!threat)
		{
			return AITask<BT>::Done("No threat!");
		}

		if (threat->IsVisibleNow())
		{
			return AITask<BT>::Done("Found an enemy!");
		}

		if (m_nav.NeedsRepath())
		{
			m_nav.ComputePathToPosition(bot, m_LKP, m_pathcost);
			m_nav.StartRepathTimer(1.0f);
		}

		m_nav.Update(bot);

		return AITask<BT>::Continue();
	}

	TaskEventResponseResult<BT> OnMoveToSuccess(BT* bot, CPath* path) override
	{
		return AITask<BT>::TryDone(PRIORITY_HIGH, "LKP reached!");
	}

	const char* GetName() const override { return "ClearLKP"; }
private:
	CT m_pathcost;
	CMeshNavigator m_nav;
	Vector m_LKP;
};



#endif // !__NAVBOT_BOT_SHARED_CLEAR_LKP_TASK_H_
