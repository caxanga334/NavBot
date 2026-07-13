#ifndef __NAVBOT_BOT_SHARED_PLUGIN_COMMAND_TASKS_
#define __NAVBOT_BOT_SHARED_PLUGIN_COMMAND_TASKS_

#include <IForwardSys.h>
#include <mods/basemod.h>
#include "bot_shared_default_combat_tasks.h"
#include "bot_shared_wait.h"
#include "bot_shared_roam.h"
#include "bot_shared_patrol_uncleared_areas.h"

template <typename BotClass, typename PathCostClass, typename BaseTaskClass>
class CBotSharedPluginWrapperTask : public BaseTaskClass
{
public:
	template <typename... Args>
	CBotSharedPluginWrapperTask(Args&&... args) :
		BaseTaskClass(std::forward<Args>(args)...)
	{
		std::string basename(BaseTaskClass::GetName());
		m_name.reserve(basename.size() + 14U);
		m_name = basename;
		m_name.insert(0, "Plugin");
	}

	TaskEventResponseResult<BotClass> OnPluginCommand(BotClass* bot, IEventListener::PluginCommandTypes type, const IEventListener::PluginCommandData& data) override
	{
		// ignore other plugin commands while this is running
		return AITask<BotClass>::TryToMaintain(PRIORITY_CRITICAL);
	}

	const char* GetName() const override { return m_name.c_str(); }

private:
	std::string m_name;
};

template <typename BotClass, typename PathCostClass>
class CBotSharedPluginScriptedBehaviorTask : public AITask<BotClass>
{
public:
	CBotSharedPluginScriptedBehaviorTask(const IEventListener::PluginCommandData& data)
	{
		constexpr std::array params = {
			SourceMod::ParamType::Param_Cell,
			SourceMod::ParamType::Param_Array,
			SourceMod::ParamType::Param_CellByRef
		};

		m_updatecallback = forwards->CreateForwardEx(nullptr, SourceMod::ExecType::ET_Event, static_cast<int>(params.size()), params.data());
		m_updatecallback->AddFunction(data.sb_update_callback);
	}

	~CBotSharedPluginScriptedBehaviorTask() override
	{
		forwards->ReleaseForward(m_updatecallback);
		m_updatecallback = nullptr;
	}

	TaskResult<BotClass> OnTaskUpdate(BotClass* bot) override
	{
		std::array<cell_t, 3> arrgoal;
		std::fill(std::begin(arrgoal), std::end(arrgoal), sp_ftoc(0.0f));
		cell_t result = static_cast<cell_t>(SourceMod::ResultType::Pl_Continue);
		cell_t routetype = static_cast<cell_t>(RouteType::DEFAULT_ROUTE);

		m_updatecallback->PushCell(bot->GetIndex());
		m_updatecallback->PushArray(arrgoal.data(), 3, SM_PARAM_COPYBACK);
		m_updatecallback->PushCellByRef(&routetype);
		m_updatecallback->Execute(&result);
		
		switch (static_cast<SourceMod::ResultType>(result))
		{
		case SourceMod::ResultType::Pl_Continue:
			m_nav.Invalidate();
			return AITask<BotClass>::Continue();
		case SourceMod::ResultType::Pl_Stop:
			return AITask<BotClass>::Done("Received Plugin_Stop from update callback!");
		default:
			break;
		}

		// update this if retreat gets implemented
		if (routetype < 0 || routetype >= static_cast<cell_t>(RouteType::RETREAT_ROUTE))
		{
			routetype = static_cast<cell_t>(RouteType::DEFAULT_ROUTE);
		}

		Vector goal(sp_ctof(arrgoal[0]), sp_ctof(arrgoal[1]), sp_ctof(arrgoal[2]));

		if (m_nav.NeedsRepath())
		{
			PathCostClass cost(bot);
			cost.SetRouteType(static_cast<RouteType>(routetype));
			m_nav.ComputePathToPosition(bot, goal, cost);
			m_nav.StartRepathTimer(2.0f);
		}

		m_nav.Update(bot);
		return AITask<BotClass>::Continue();
	}

	TaskEventResponseResult<BotClass> OnPluginCommand(BotClass* bot, IEventListener::PluginCommandTypes type, const IEventListener::PluginCommandData& data) override
	{
		// ignore other plugin commands while this is running
		return AITask<BotClass>::TryToMaintain(PRIORITY_CRITICAL);
	}

	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_YES; }
	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override { return ANSWER_YES; }
	QueryAnswerType ShouldHurry(CBaseBot* me) override { return ANSWER_YES; }
	QueryAnswerType ShouldRetreat(CBaseBot* me) override { return ANSWER_NO; }

	const char* GetName() const override { return "ScriptedBehavior"; }
private:
	CMeshNavigator m_nav;
	SourceMod::IChangeableForward* m_updatecallback;
};

template <typename BotClass, typename PathCostClass>
class CBotSharedPluginAttackMoveTask : public AITask<BotClass>
{
public:
	CBotSharedPluginAttackMoveTask(const Vector& goal) :
		m_goal(goal)
	{
		m_pathFailures = 0;
		m_failLimit = extmanager->GetMod()->GetModSettings()->GetStuckGiveUpThreshold();
	}

	TaskResult<BotClass> OnTaskStart(BotClass* bot, AITask<BotClass>* pastTask) override
	{
		PathCostClass cost(bot);

		if (!m_nav.ComputePathToPosition(bot, m_goal, cost))
		{
			return AITask<BotClass>::Done("No path to goal!");
		}

		return AITask<BotClass>::Continue();
	}

	TaskResult<BotClass> OnTaskUpdate(BotClass* bot) override
	{
		const CKnownEntity* threat = bot->GetSensorInterface()->GetPrimaryKnownThreat(ISensor::ONLY_VISIBLE_THREATS);

		if (threat)
		{
			return AITask<BotClass>::PauseFor(new CBotSharedDefaultCombatBehaviorTask<BotClass, PathCostClass>, "Attacking visible threat!");
		}

		if (m_nav.NeedsRepath())
		{
			PathCostClass cost(bot);

			if (!m_nav.ComputePathToPosition(bot, m_goal, cost))
			{
				if (++m_pathFailures >= m_failLimit)
				{
					return AITask<BotClass>::Done("No path to goal!");
				}
			}

			m_nav.StartRepathTimer();
		}


		m_nav.Update(bot);
		return AITask<BotClass>::Continue();
	}

	bool OnTaskPause(BotClass* bot, AITask<BotClass>* nextTask) override
	{
		m_nav.Invalidate();
		return true;
	}

	TaskEventResponseResult<BotClass> OnStuck(BotClass* bot) override
	{
		m_nav.Invalidate();
		
		if (++m_pathFailures >= m_failLimit)
		{
			return AITask<BotClass>::TryDone(PRIORITY_CRITICAL, "Failure limit reached, giving up!");
		}

		return AITask<BotClass>::TryToMaintain();
	}

	TaskEventResponseResult<BotClass> OnMoveToSuccess(BotClass* bot, CPath* path) override
	{
		return AITask<BotClass>::TryDone(PRIORITY_CRITICAL, "Goal reached!");
	}

	TaskEventResponseResult<BotClass> OnPluginCommand(BotClass* bot, IEventListener::PluginCommandTypes type, const IEventListener::PluginCommandData& data) override
	{
		// ignore other plugin commands while this is running
		return AITask<BotClass>::TryToMaintain(PRIORITY_CRITICAL);
	}

	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_YES; }
	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override { return ANSWER_YES; }
	QueryAnswerType ShouldRetreat(CBaseBot* me) override { return ANSWER_NO; }

	const char* GetName() const override { return "PluginAttackMove"; }

private:
	CMeshNavigator m_nav;
	Vector m_goal;
	int m_pathFailures;
	int m_failLimit;
};

template <typename BotClass, typename PathCostClass>
class CBotSharedPluginMoveToTask : public AITask<BotClass>
{
public:
	CBotSharedPluginMoveToTask(const Vector& goal) :
		m_goal(goal)
	{
		m_pathFailures = 0;
		m_failLimit = extmanager->GetMod()->GetModSettings()->GetStuckGiveUpThreshold();
	}

	TaskResult<BotClass> OnTaskStart(BotClass* bot, AITask<BotClass>* pastTask) override
	{
		PathCostClass cost(bot);

		if (!m_nav.ComputePathToPosition(bot, m_goal, cost))
		{
			return AITask<BotClass>::Done("No path to goal!");
		}

		return AITask<BotClass>::Continue();
	}

	TaskResult<BotClass> OnTaskUpdate(BotClass* bot) override
	{
		if (m_nav.NeedsRepath())
		{
			PathCostClass cost(bot);

			if (!m_nav.ComputePathToPosition(bot, m_goal, cost))
			{
				if (++m_pathFailures >= m_failLimit)
				{
					return AITask<BotClass>::Done("No path to goal!");
				}
			}

			m_nav.StartRepathTimer();
		}


		m_nav.Update(bot);
		return AITask<BotClass>::Continue();
	}

	bool OnTaskPause(BotClass* bot, AITask<BotClass>* nextTask) override
	{
		m_nav.Invalidate();
		return true;
	}

	TaskEventResponseResult<BotClass> OnStuck(BotClass* bot) override
	{
		m_nav.Invalidate();

		if (++m_pathFailures >= m_failLimit)
		{
			return AITask<BotClass>::TryDone(PRIORITY_CRITICAL, "Failure limit reached, giving up!");
		}

		return AITask<BotClass>::TryToMaintain();
	}

	TaskEventResponseResult<BotClass> OnMoveToSuccess(BotClass* bot, CPath* path) override
	{
		return AITask<BotClass>::TryDone(PRIORITY_CRITICAL, "Goal reached!");
	}

	TaskEventResponseResult<BotClass> OnPluginCommand(BotClass* bot, IEventListener::PluginCommandTypes type, const IEventListener::PluginCommandData& data) override
	{
		// ignore other plugin commands while this is running
		return AITask<BotClass>::TryToMaintain(PRIORITY_CRITICAL);
	}

	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_YES; }
	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override { return ANSWER_YES; }
	QueryAnswerType ShouldHurry(CBaseBot* me) override { return ANSWER_YES; }
	QueryAnswerType ShouldRetreat(CBaseBot* me) override { return ANSWER_NO; }

	const char* GetName() const override { return "PluginMoveToPosition"; }

private:
	CMeshNavigator m_nav;
	Vector m_goal;
	int m_pathFailures;
	int m_failLimit;
};

template <typename BotClass, typename PathCostClass>
class CBotSharedPluginSeekAndDestroyTask : public AITask<BotClass>
{
public:
	CBotSharedPluginSeekAndDestroyTask(CBaseEntity* entity) :
		m_target(entity)
	{
		m_pathFailures = 0;
		m_failLimit = extmanager->GetMod()->GetModSettings()->GetStuckGiveUpThreshold();
	}

	TaskResult<BotClass> OnTaskUpdate(BotClass* bot) override
	{
		CBaseEntity* target = m_target.Get();

		if (!target)
		{
			return AITask<BotClass>::Done("Target entity is NULL!");
		}

		if (modhelpers->IsDead(target))
		{
			return AITask<BotClass>::Done("Target entity is dead!");
		}

		const ISensor* sensor = bot->GetSensorInterface();

		if (!m_pto.IsSet() && sensor->IsAbleToSee(target))
		{
			m_pto.Set(bot, target);
		}

		if (m_nav.NeedsRepath())
		{
			PathCostClass cost(bot);
			Vector goal = UtilHelpers::getWorldSpaceCenter(target);

			if (!m_nav.ComputePathToPosition(bot, goal, cost))
			{
				if (++m_pathFailures >= m_failLimit)
				{
					return AITask<BotClass>::Done("No path to goal!");
				}
			}

			m_nav.StartRepathTimer();
		}

		m_nav.Update(bot);
		return AITask<BotClass>::Continue();
	}

	bool OnTaskPause(BotClass* bot, AITask<BotClass>* nextTask) override
	{
		m_nav.Invalidate();
		return true;
	}

	TaskEventResponseResult<BotClass> OnStuck(BotClass* bot) override
	{
		m_nav.Invalidate();

		if (++m_pathFailures >= m_failLimit)
		{
			return AITask<BotClass>::TryDone(PRIORITY_CRITICAL, "Failure limit reached, giving up!");
		}

		return AITask<BotClass>::TryToMaintain();
	}

	TaskEventResponseResult<BotClass> OnPluginCommand(BotClass* bot, IEventListener::PluginCommandTypes type, const IEventListener::PluginCommandData& data) override
	{
		// ignore other plugin commands while this is running
		return AITask<BotClass>::TryToMaintain(PRIORITY_CRITICAL);
	}

	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override { return ANSWER_YES; }
	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override { return ANSWER_YES; }
	QueryAnswerType ShouldHurry(CBaseBot* me) override { return ANSWER_YES; }
	QueryAnswerType ShouldRetreat(CBaseBot* me) override { return ANSWER_NO; }

	const char* GetName() const override { return "PluginSeekAndDestroy"; }

private:
	CHandle<CBaseEntity> m_target;
	CMeshNavigator m_nav;
	int m_pathFailures;
	int m_failLimit;
	sensorutils::PrimaryThreatOverride<BotClass> m_pto;
};

template <typename BotClass, typename PathCostClass>
class CBotSharedPluginUseEntityTask : public AITask<BotClass>
{
public:
	CBotSharedPluginUseEntityTask(CBaseEntity* entity) :
		m_target(entity)
	{
		m_pathFailures = 0;
		m_failLimit = extmanager->GetMod()->GetModSettings()->GetStuckGiveUpThreshold();
	}

	TaskResult<BotClass> OnTaskUpdate(BotClass* bot) override
	{
		CBaseEntity* target = m_target.Get();

		if (!target)
		{
			return AITask<BotClass>::Done("Target entity is NULL!");
		}

		if (m_timeout.HasStarted() && m_timeout.IsElapsed())
		{
			return AITask<BotClass>::Done("Task timed out!");
		}

		if (m_nav.NeedsRepath())
		{
			PathCostClass cost(bot);
			Vector goal = UtilHelpers::getWorldSpaceCenter(target);

			if (!m_nav.ComputePathToPosition(bot, goal, cost))
			{
				if (++m_pathFailures >= m_failLimit)
				{
					return AITask<BotClass>::Done("No path to goal!");
				}
			}

			m_nav.StartRepathTimer();
		}

		m_nav.Update(bot);

		Vector eyePos = bot->GetEyeOrigin();
		Vector center = UtilHelpers::getWorldSpaceCenter(target);
		const float range = (eyePos - center).Length();

		if (range <= CBaseExtPlayer::PLAYER_USE_RADIUS)
		{
			if (!m_timeout.HasStarted())
			{
				m_timeout.Start(5.0f);
			}

			if (!modhelpers->IsUseObstructed(bot->GetEntity(), target))
			{
				IPlayerController* control = bot->GetControlInterface();

				control->AimAt(center, IPlayerController::LookPriority::LOOK_CRITICAL, 0.5f, "Looking at target entity to use.");

				if (control->IsAimOnTarget())
				{
					control->PressUseButton();
					return AITask<BotClass>::Done("Use button pressed!");
				}
			}
		}

		return AITask<BotClass>::Continue();
	}

	bool OnTaskPause(BotClass* bot, AITask<BotClass>* nextTask) override
	{
		m_nav.Invalidate();
		return true;
	}

	TaskEventResponseResult<BotClass> OnStuck(BotClass* bot) override
	{
		m_nav.Invalidate();

		if (++m_pathFailures >= m_failLimit)
		{
			return AITask<BotClass>::TryDone(PRIORITY_CRITICAL, "Failure limit reached, giving up!");
		}

		return AITask<BotClass>::TryToMaintain();
	}

	TaskEventResponseResult<BotClass> OnPluginCommand(BotClass* bot, IEventListener::PluginCommandTypes type, const IEventListener::PluginCommandData& data) override
	{
		// ignore other plugin commands while this is running
		return AITask<BotClass>::TryToMaintain(PRIORITY_CRITICAL);
	}

	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override
	{
		CBaseEntity* target = m_target.Get();

		if (target)
		{
			Vector eyePos = me->GetEyeOrigin();
			Vector center = UtilHelpers::getWorldSpaceCenter(target);
			const float range = (eyePos - center).Length();

			if (range <= CBaseExtPlayer::PLAYER_USE_RADIUS * 2.0f)
			{
				return ANSWER_NO;
			}
		}

		return ANSWER_YES;
	}

	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override { return ANSWER_YES; }
	QueryAnswerType ShouldHurry(CBaseBot* me) override { return ANSWER_YES; }
	QueryAnswerType ShouldRetreat(CBaseBot* me) override { return ANSWER_NO; }

	const char* GetName() const override { return "PluginUseEntity"; }

private:
	CountdownTimer m_timeout;
	CHandle<CBaseEntity> m_target;
	CMeshNavigator m_nav;
	int m_pathFailures;
	int m_failLimit;
};

namespace plugincommandtask
{
	template <typename TaskClass, typename BotClass, typename PathCostClass>
	inline TaskEventResponseResult<BotClass> ImplementPluginCommandTasks(TaskClass* task, BotClass* bot, IEventListener::PluginCommandTypes type, const IEventListener::PluginCommandData& data)
	{
		using namespace std::literals::string_view_literals;
		constexpr auto REASON = "Responding to plugin command!"sv;
		constexpr auto PRIORITY = EventResultPriorityType::PRIORITY_CRITICAL;

		switch (type)
		{
		case IEventListener::PluginCommandTypes::PLUGINCMD_SCRIPTED:
		{
			return task->TryPauseFor(new CBotSharedPluginScriptedBehaviorTask<BotClass, PathCostClass>(data), PRIORITY, REASON.data());
		}
		case IEventListener::PluginCommandTypes::PLUGINCMD_ATTACK_MOVE:
		{
			return task->TryPauseFor(new CBotSharedPluginAttackMoveTask<BotClass, PathCostClass>(data.movegoal), PRIORITY, REASON.data());
		}
		case IEventListener::PluginCommandTypes::PLUGINCMD_MOVE_TO:
		{
			return task->TryPauseFor(new CBotSharedPluginMoveToTask<BotClass, PathCostClass>(data.movegoal), PRIORITY, REASON.data());
		}
		case IEventListener::PluginCommandTypes::PLUGINCMD_SEEK_AND_DESTROY:
		{
			return task->TryPauseFor(new CBotSharedPluginSeekAndDestroyTask<BotClass, PathCostClass>(data.entdata.Get()), PRIORITY, REASON.data());
		}
		case IEventListener::PluginCommandTypes::PLUGINCMD_USE_ENTITY:
		{
			return task->TryPauseFor(new CBotSharedPluginUseEntityTask<BotClass, PathCostClass>(data.entdata.Get()), PRIORITY, REASON.data());
		}
		case IEventListener::PluginCommandTypes::PLUGINCMD_WAIT:
		{
			using WAITTASK = CBotSharedWaitTask<BotClass>;
			return task->TryPauseFor(new CBotSharedPluginWrapperTask<BotClass, PathCostClass, WAITTASK>(data.fldata), PRIORITY, REASON.data());
		}
		case IEventListener::PluginCommandTypes::PLUGINCMD_PATROL:
		{
			using PATROLTASK = CBotSharedPatrolUnclearedAreasTask<BotClass, PathCostClass>;
			CNavArea* goalArea = nullptr;

			if (PATROLTASK::IsPossible(bot, &goalArea))
			{
				return task->TryPauseFor(new CBotSharedPluginWrapperTask<BotClass, PathCostClass, PATROLTASK>(goalArea), PRIORITY, REASON.data());
			}

			return task->TryContinue();
		}
		case IEventListener::PluginCommandTypes::PLUGINCMD_ROAM:
		{
			using ROAMTASK = CBotSharedRoamTask<BotClass, PathCostClass>;
			return task->TryPauseFor(new CBotSharedPluginWrapperTask<BotClass, PathCostClass, ROAMTASK>(bot, data.fldata), PRIORITY, REASON.data());
		}
		default:
			return task->TryContinue();
		}
	}
}

#endif // !__NAVBOT_BOT_SHARED_PLUGIN_COMMAND_TASKS_
