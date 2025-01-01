#include <extension.h>
#include <manager.h>
#include <extplayer.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <util/librandom.h>
#include <util/sdkcalls.h>
#include <bot/interfaces/base_interface.h>
#include <bot/interfaces/knownentity.h>
#include <bot/interfaces/playerinput.h>
#include <bot/interfaces/tasks.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <sdkports/sdk_takedamageinfo.h>
#include <sdkports/sdk_traces.h>
#include "basebot_behavior.h"
#include "basebot.h"

class CBaseBotTestTask : public AITask<CBaseBot>
{
public:
	TaskResult<CBaseBot> OnTaskUpdate(CBaseBot* bot) override;
	TaskResult<CBaseBot> OnTaskResume(CBaseBot* bot, AITask<CBaseBot>* pastTask) override;
	TaskEventResponseResult<CBaseBot> OnTestEventPropagation(CBaseBot* bot) override;
	QueryAnswerType ShouldFreeRoam(CBaseBot* me) override;
	const char* GetName() const override { return "CBaseBotTestTask"; }
};

class CBaseBotPathTestTask : public AITask<CBaseBot>
{
public:
	CBaseBotPathTestTask()
	{
		m_failcount = 0;
	}

	TaskResult<CBaseBot> OnTaskStart(CBaseBot* bot, AITask<CBaseBot>* pastTask) override;
	TaskResult<CBaseBot> OnTaskUpdate(CBaseBot* bot) override;
	TaskEventResponseResult<CBaseBot> OnMoveToSuccess(CBaseBot* bot, CPath* path) override;
	TaskEventResponseResult<CBaseBot> OnMoveToFailure(CBaseBot* bot, CPath* path, IEventListener::MovementFailureType reason) override;
	const char* GetName() const override { return "CBaseBotPathTestTask"; }

private:
	CMeshNavigator m_nav;
	Vector m_goal;
	int m_failcount;
};

class CBaseBotSwitchTestTask : public AITask<CBaseBot>
{
public:
	TaskResult<CBaseBot> OnTaskStart(CBaseBot* bot, AITask<CBaseBot>* pastTask) override;
	TaskResult<CBaseBot> OnTaskUpdate(CBaseBot* bot) override;
	const char* GetName() const override { return "CBaseBotSwitchTestTask"; }
};

TaskResult<CBaseBot> CBaseBotTestTask::OnTaskUpdate(CBaseBot* bot)
{
	// rootconsole->ConsolePrint("AI Task -- Update");
	return Continue();
}

TaskResult<CBaseBot> CBaseBotTestTask::OnTaskResume(CBaseBot* bot, AITask<CBaseBot>* pastTask)
{
	rootconsole->ConsolePrint("CBaseBotTestTask::OnTaskResume");
	return SwitchTo(new CBaseBotSwitchTestTask, "Testing Task Switch!");
}

TaskEventResponseResult<CBaseBot> CBaseBotTestTask::OnTestEventPropagation(CBaseBot* bot)
{
	rootconsole->ConsolePrint("AI Event -- OnTestEventPropagation");
	return TryPauseFor(new CBaseBotPathTestTask, PRIORITY_HIGH, "Event pause test!");
}

QueryAnswerType CBaseBotTestTask::ShouldFreeRoam(CBaseBot* me)
{
	rootconsole->ConsolePrint("AI Query -- ShouldFreeRoam");
	return ANSWER_YES;
}

TaskResult<CBaseBot> CBaseBotPathTestTask::OnTaskStart(CBaseBot* bot, AITask<CBaseBot>* pastTask)
{
	edict_t* host = gamehelpers->EdictOfIndex(1); // get listen server host
	CBaseExtPlayer player(host);
	ShortestPathCost cost;

	m_goal = player.GetAbsOrigin();
	m_nav.SetSkipAheadDistance(350.0f);

	bool result = m_nav.ComputePathToPosition(bot, m_goal, cost);

	if (result == false)
	{
		rootconsole->ConsolePrint("CBaseBotPathTestTask::OnTaskStart path failed!");
		return Done("Failed to find a path!");
	}

	return Continue();
}

TaskResult<CBaseBot> CBaseBotPathTestTask::OnTaskUpdate(CBaseBot* bot)
{
	if (m_nav.IsValid() == false)
	{
		return Done("My Path is invalid!");
	}

	if (m_nav.GetAge() > 1.0f)
	{
		ShortestPathCost cost;
		bool result = m_nav.ComputePathToPosition(bot, m_goal, cost);

		if (result == false)
		{
			return Done("No Path!");
		}
	}

	m_nav.Update(bot);
	return Continue();
}

TaskEventResponseResult<CBaseBot> CBaseBotPathTestTask::OnMoveToSuccess(CBaseBot* bot, CPath* path)
{
	return TryDone(PRIORITY_HIGH, "Returning to previous task!");
}

TaskEventResponseResult<CBaseBot> CBaseBotPathTestTask::OnMoveToFailure(CBaseBot* bot, CPath* path, IEventListener::MovementFailureType reason)
{
	if (++m_failcount > 20)
	{
		return TryDone(PRIORITY_HIGH, "Too many pathing failures, giving up!");
	}

	return TryContinue(PRIORITY_DONT_CARE);
}

TaskResult<CBaseBot> CBaseBotSwitchTestTask::OnTaskStart(CBaseBot* bot, AITask<CBaseBot>* pastTask)
{
	rootconsole->ConsolePrint("CBaseBotSwitchTestTask::OnTaskStart");

	if (pastTask != nullptr)
	{
		rootconsole->ConsolePrint("%p", pastTask);
	}

	return Continue();
}

TaskResult<CBaseBot> CBaseBotSwitchTestTask::OnTaskUpdate(CBaseBot* bot)
{
	rootconsole->ConsolePrint("Hello from CBaseBotSwitchTestTask::OnTaskUpdate!");
	return SwitchTo(new CBaseBotTestTask, "Returning back to main task!");
}

CBaseBotBehavior::CBaseBotBehavior(CBaseBot* bot) : IBehavior(bot)
{
	m_manager = std::make_unique<AITaskManager<CBaseBot>>(new CBaseBotTestTask);
	m_listeners.reserve(2);
	m_listeners.push_back(m_manager.get());
}

CBaseBotBehavior::~CBaseBotBehavior()
{
}

void CBaseBotBehavior::Reset()
{
	m_listeners.clear();

	m_manager = nullptr;
	m_manager = std::make_unique<AITaskManager<CBaseBot>>(new CBaseBotTestTask);

	m_listeners.push_back(m_manager.get());
}

void CBaseBotBehavior::Update()
{
	m_manager->Update(GetBot());
}

std::vector<IEventListener*>* CBaseBotBehavior::GetListenerVector()
{
	if (!m_manager)
	{
		return nullptr;
	}

	static std::vector<IEventListener*> listeners;
	listeners.clear();
	listeners.push_back(m_manager.get());
	return &listeners;
}