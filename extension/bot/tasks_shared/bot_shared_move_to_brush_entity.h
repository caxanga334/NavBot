#ifndef __NAVBOT_BOT_SHARED_MOVE_TO_BRUSH_ENTITY_TASK_H_
#define __NAVBOT_BOT_SHARED_MOVE_TO_BRUSH_ENTITY_TASK_H_

#include <extension.h>
#include <util/helpers.h>
#include <util/librandom.h>
#include <bot/basebot.h>
#include <bot/basebot_pathcost.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <sdkports/sdk_timers.h>
#include <sdkports/sdk_traces.h>
#include <navmesh/nav_mesh.h>

/**
 * @brief Generic helper tasks to move bots towards a trigger entity.
 * @tparam BT Bot class.
 * @tparam CT Bot path class.
 * @tparam autoEnd If true, automatically ends the task when OnMoveToSuccess is invoked.
 */
template <typename BT, typename CT = CBaseBotPathCost, bool autoEnd = false>
class CBotSharedMoveToBrushEntityTask : public AITask<BT>
{
public:
	CBotSharedMoveToBrushEntityTask(BT* bot, CBaseEntity* goal, const float recomputeGoalInterval = 2.0f) :
		m_pathcost(bot), m_cachedBrushPosition(0.0f, 0.0f, 0.0f), m_goalEnt(goal)
	{
		m_moveFailures = 0;
		m_recomputeDelay = recomputeGoalInterval;
	}

	TaskResult<BT> OnTaskStart(BT* bot, AITask<BT>* pastTask) override;
	TaskResult<BT> OnTaskUpdate(BT* bot) override;

	TaskEventResponseResult<BT> OnStuck(BT* bot) override;
	TaskEventResponseResult<BT> OnMoveToFailure(BT* bot, CPath* path, IEventListener::MovementFailureType reason) override;
	TaskEventResponseResult<BT> OnMoveToSuccess(BT* bot, CPath* path) override;

	const char* GetName() const override { return "MoveToBrushEntity"; }
private:
	CT m_pathcost;
	CMeshNavigator m_nav;
	CountdownTimer m_recomputeBrushPositionTimer;
	Vector m_cachedBrushPosition;
	CHandle<CBaseEntity> m_goalEnt;
	int m_moveFailures;
	float m_recomputeDelay;

	void UpdateEntityPosition(CBaseEntity* ent, const bool isInit = false);
};

template<typename BT, typename CT, bool autoEnd>
inline TaskResult<BT> CBotSharedMoveToBrushEntityTask<BT, CT, autoEnd>::OnTaskStart(BT* bot, AITask<BT>* pastTask)
{
	CBaseEntity* ent = m_goalEnt.Get();

	if (!ent)
	{
		return AITask<BT>::Done("Goal entity is NULL!");
	}

	UpdateEntityPosition(ent, true);

	return AITask<BT>::Continue();
}

template<typename BT, typename CT, bool autoEnd>
inline TaskResult<BT> CBotSharedMoveToBrushEntityTask<BT, CT, autoEnd>::OnTaskUpdate(BT* bot)
{
	CBaseEntity* ent = m_goalEnt.Get();

	if (!ent)
	{
		return AITask<BT>::Done("Goal entity is NULL!");
	}

	// Currently touching the brush entity, stop moving
	if (trace::pointwithin(ent, bot->GetAbsOrigin()))
	{
		return AITask<BT>::Continue();
	}

	if (m_recomputeBrushPositionTimer.IsElapsed())
	{
		m_recomputeBrushPositionTimer.Start(m_recomputeDelay);
		UpdateEntityPosition(ent);
	}

	if (!m_nav.IsValid() || m_nav.NeedsRepath())
	{
		if (!m_nav.ComputePathToPosition(bot, m_cachedBrushPosition, m_pathcost, 0.0f, true))
		{
			if (++m_moveFailures >= 20)
			{
				return AITask<BT>::Done("Failed to compute a position to the goal entity!");
			}

			UpdateEntityPosition(ent, true); // try again
		}
	}

	m_nav.Update(bot);

	return AITask<BT>::Continue();
}

template<typename BT, typename CT, bool autoEnd>
inline TaskEventResponseResult<BT> CBotSharedMoveToBrushEntityTask<BT, CT, autoEnd>::OnStuck(BT* bot)
{
	m_nav.Invalidate();
	m_nav.ForceRepath();

	if (++m_moveFailures >= 20)
	{
		return AITask<BT>::TryDone(PRIORITY_HIGH, "Too many pathing failures!");
	}

	return AITask<BT>::TryContinue(PRIORITY_LOW);
}

template<typename BT, typename CT, bool autoEnd>
inline TaskEventResponseResult<BT> CBotSharedMoveToBrushEntityTask<BT, CT, autoEnd>::OnMoveToFailure(BT* bot, CPath* path, IEventListener::MovementFailureType reason)
{
	m_nav.Invalidate();
	m_nav.ForceRepath();

	if (++m_moveFailures >= 20)
	{
		return AITask<BT>::TryDone(PRIORITY_HIGH, "Too many pathing failures!");
	}

	return AITask<BT>::TryContinue(PRIORITY_LOW);
}

template<typename BT, typename CT, bool autoEnd>
inline TaskEventResponseResult<BT> CBotSharedMoveToBrushEntityTask<BT, CT, autoEnd>::OnMoveToSuccess(BT* bot, CPath* path)
{
	if constexpr (autoEnd)
	{
		return AITask<BT>::TryDone(PRIORITY_HIGH, "Goal reached!");
	}

	return AITask<BT>::TryContinue(PRIORITY_LOW);
}

template<typename BT, typename CT, bool autoEnd>
inline void CBotSharedMoveToBrushEntityTask<BT, CT, autoEnd>::UpdateEntityPosition(CBaseEntity* ent, const bool isInit)
{
	if (!isInit)
	{
		// current position is still valid, don't update
		if (trace::pointwithin(ent, m_cachedBrushPosition))
		{
			return;
		}
	}

	std::vector<CNavArea*> touchingAreas;

	TheNavMesh->CollectAreasTouchingEntity(ent, true, touchingAreas);

	if (touchingAreas.empty())
	{
		m_cachedBrushPosition = UtilHelpers::getWorldSpaceCenter(ent);
		return;
	}

	CNavArea* area = librandom::utils::GetRandomElementFromVector(touchingAreas);
	m_cachedBrushPosition = area->GetCenter();
}

#endif // !__NAVBOT_BOT_SHARED_MOVE_TO_BRUSH_ENTITY_TASK_H_