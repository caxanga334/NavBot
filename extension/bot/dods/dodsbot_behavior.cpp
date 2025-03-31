#include <extension.h>
#include "tasks/dodsbot_main_task.h"
#include "dodsbot.h"
#include "dodsbot_behavior.h"

CDoDSBotBehavior::CDoDSBotBehavior(CBaseBot* bot) :
	IBehavior(bot)
{
	m_manager = new AITaskManager<CDoDSBot>(new CDoDSBotMainTask);
	m_listeners.reserve(2);
}

CDoDSBotBehavior::~CDoDSBotBehavior()
{
	m_listeners.clear();
	delete m_manager;
	m_manager = nullptr;
}

void CDoDSBotBehavior::Reset()
{
	m_listeners.clear();
	delete m_manager;
	m_manager = new AITaskManager<CDoDSBot>(new CDoDSBotMainTask);
}

void CDoDSBotBehavior::Update()
{
	m_manager->Update(GetBot<CDoDSBot>());
}

std::vector<IEventListener*>* CDoDSBotBehavior::GetListenerVector()
{
	if (m_manager == nullptr)
	{
		return nullptr;
	}

	m_listeners.clear();
	m_listeners.push_back(m_manager);

	return &m_listeners;
}

IDecisionQuery* CDoDSBotBehavior::GetDecisionQueryResponder()
{
	return m_manager;
}
