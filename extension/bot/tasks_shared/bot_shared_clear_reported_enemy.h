#ifndef __NAVBOT_BOT_SHARED_CLEAR_REPORTED_ENEMY_TASK_H_
#define __NAVBOT_BOT_SHARED_CLEAR_REPORTED_ENEMY_TASK_H_

#include <manager.h>
#include <mods/basemod.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <bot/bot_shared_utils.h>
#include "bot_shared_roam.h"

template <typename BT, typename CT>
class CBotSharedClearReportedEnemyTask : public AITask<BT>
{
public:
	static bool IsPossible(BT* bot, CBaseEntity** target)
	{
		const ISharedBotMemory* sbm = bot->GetSharedMemoryInterface();
		std::vector<const ISharedBotMemory::ReportedEntityData*> entities;

		if (sbm->CollectReportedEntities(entities) < 1)
		{
			// no entity collected
			return false;
		}

		botsharedutils::IsReachableAreas collector(bot, 8192.0f);
		collector.Execute();

		if (collector.IsCollectedAreasEmpty())
		{
			// bot stuck outside nav mesh?
			return false;
		}

		CBaseEntity* selected = nullptr;
		float nearest = std::numeric_limits<float>::max();

		// all instances in this vector are guaranteed to be valid
		for (const ISharedBotMemory::ReportedEntityData* instance : entities)
		{
			// already searched, skip
			if (instance->IsCleared())
			{
				continue;
			}

			CBaseEntity* entity = instance->GetEntity();

			// must be an enemy
			if (!bot->GetSensorInterface()->IsEnemy(entity))
			{
				continue;
			}

			const CNavArea* area = instance->GetLastKnownNavArea();

			// outside nav mesh
			if (!area)
			{
				continue;
			}

			auto node = collector.GetNodeForArea(area);

			// not reachable
			if (!node)
			{
				continue;
			}

			const float distance = node->GetTravelCostFromStart();

			if (distance < nearest)
			{
				nearest = distance;
				selected = instance->GetEntity();
			}
		}

		// all instances are not reachable
		if (!selected)
		{
			return false;
		}

		*target = selected;
		return true;
	}

	CBotSharedClearReportedEnemyTask(CBaseEntity* target) :
		m_entity(target), m_goal(0.0f, 0.0f, 0.0f), m_stuckthreshold(extmanager->GetMod()->GetModSettings()->GetStuckGiveUpThreshold())
	{
	}

	TaskResult<BT> OnTaskStart(BT* bot, AITask<BT>* pastTask) override
	{
		CBaseEntity* target = m_entity.Get();

		if (!target)
		{
			return AITask<BT>::Done("Investigate entity is NULL!");
		}

		const ISharedBotMemory::ReportedEntityData* red = bot->GetSharedMemoryInterface()->GetReportedEntityInstance(target);

		if (!red)
		{
			return AITask<BT>::Done("Investigate entity is no longer in shared memory!");
		}

		m_goal = red->GetLastKnownPosition();
		m_tac.Enable(bot->GetCombatInterface());

		return AITask<BT>::Continue();
	}

	TaskResult<BT> OnTaskUpdate(BT* bot) override
	{
		CBaseEntity* target = m_entity.Get();

		if (!target)
		{
			return AITask<BT>::Done("Investigate entity is NULL!");
		}

		const ISharedBotMemory::ReportedEntityData* red = bot->GetSharedMemoryInterface()->GetReportedEntityInstance(target);

		if (!red)
		{
			return AITask<BT>::Done("Investigate entity is no longer in shared memory!");
		}

		if (red->IsCleared())
		{
			return AITask<BT>::Done("Another teammate has investigated this!");
		}

		if (bot->GetSensorInterface()->IsAbleToSee(red->GetLastKnownPosition()))
		{
			bot->GetSharedMemoryInterface()->UpdateReportedEntityAsCleared(target);
			const Vector& pos = UtilHelpers::getEntityOrigin(target);

			// if the bot is somewhat close to the target current position
			if (bot->GetRangeTo(pos) <= 1024.0f)
			{
				// random chance to move to their exact position
				if (bot->GetDifficultyProfile()->GetGameAwareness() >= 90 && CBaseBot::s_botrng.GetRandomChance(33))
				{
					return AITask<BT>::SwitchTo(new CBotSharedRoamTask<BT, CT>(bot, pos, 30.0f, true, true), "Moving to target current position (random chance)!");
				}
			}
		}

		if (m_nav.NeedsRepath())
		{
			CT cost(bot);
			cost.SetRouteType(RouteType::FASTEST_ROUTE);
			m_nav.ComputePathToPosition(bot, m_goal, cost);
			m_nav.StartRepathTimer();
		}

		m_nav.Update(bot);
		return AITask<BT>::Continue();
	}

	bool OnTaskPause(BT* bot, AITask<BT>* nextTask) override
	{
		m_tac.Disable();
		return true;
	}

	void OnTaskEnd(BT* bot, AITask<BT>* nextTask) override
	{
		m_tac.Disable();
	}

	TaskResult<BT> OnTaskResume(BT* bot, AITask<BT>* pastTask) override
	{
		m_tac.Enable(bot->GetCombatInterface());
		return AITask<BT>::Continue();
	}

	TaskEventResponseResult<BT> OnNavAreaChanged(BT* bot, CNavArea* oldArea, CNavArea* newArea) override
	{
		if (newArea)
		{
			newArea->MarkAsCleared(bot->GetCurrentTeamIndex());
		}

		return AITask<BT>::TryToMaintain(PRIORITY_LOW);
	}

	TaskEventResponseResult<BT> OnStuck(BT* bot) override
	{
		if (bot->GetMovementInterface()->GetStuckCount() > m_stuckthreshold)
		{
			return AITask<BT>::TryDone(PRIORITY_HIGH, "Got stuck!");
		}

		m_nav.Invalidate();
		return AITask<BT>::TryToMaintain(PRIORITY_LOW);
	}

	TaskEventResponseResult<BT> OnMoveToSuccess(BT* bot, CPath* path) override
	{
		if (bot->GetLastKnownNavArea() != nullptr)
		{
			bot->GetLastKnownNavArea()->MarkAsCleared(bot->GetCurrentTeamIndex());
		}

		CBaseEntity* target = m_entity.Get();

		if (target)
		{
			bot->GetSharedMemoryInterface()->UpdateReportedEntityAsCleared(target);

			const Vector& pos = UtilHelpers::getEntityOrigin(target);

			// if the bot is somewhat close to the target current position
			if (bot->GetRangeTo(pos) <= 1024.0f)
			{
				// random chance to move to their exact position
				if (bot->GetDifficultyProfile()->GetGameAwareness() >= 90 && CBaseBot::s_botrng.GetRandomChance(33))
				{
					return AITask<BT>::TrySwitchTo(new CBotSharedRoamTask<BT, CT>(bot, pos, 30.0f, true, true), PRIORITY_MEDIUM, "Moving to target current position (random chance)!");
				}
			}
		}

		return AITask<BT>::TryDone(PRIORITY_HIGH, "Goal reached!");
	}

	TaskEventResponseResult<BT> OnSight(BT* bot, CBaseEntity* subject) override
	{
		if (bot->GetSensorInterface()->IsEnemy(subject))
		{
			CBaseEntity* target = m_entity.Get();

			// spotted my target, update shared memory about them
			if (target && target == subject)
			{
				bot->GetSharedMemoryInterface()->ReportEntityVisible(target);
			}

			return AITask<BT>::TryDone(PRIORITY_HIGH, "Enemy spotted!");
		}

		return AITask<BT>::TryToMaintain(PRIORITY_LOW);
	}

	const char* GetName() const override { return "ClearReportedEnemy"; }
private:
	CHandle<CBaseEntity> m_entity;
	CMeshNavigator m_nav;
	combatutils::ToggleAreaClearing m_tac;
	Vector m_goal;
	int m_stuckthreshold;
};


#endif // !__NAVBOT_BOT_SHARED_CLEAR_REPORTED_ENEMY_TASK_H_
