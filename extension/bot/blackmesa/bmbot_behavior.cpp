#include <extension.h>
#include "bmbot.h"
#include "bmbot_behavior.h"
#include <bot/blackmesa/tasks/bmbot_main_task.h>

CBlackMesaBotBehavior::CBlackMesaBotBehavior(CBaseBot* bot) :
	IBehavior(bot)
{
	m_manager = std::make_unique<AITaskManager<CBlackMesaBot>>(new CBlackMesaBotMainTask);
	m_listeners.reserve(2);
}

CBlackMesaBotBehavior::~CBlackMesaBotBehavior()
{
}

void CBlackMesaBotBehavior::Reset()
{
	m_manager = std::make_unique<AITaskManager<CBlackMesaBot>>(new CBlackMesaBotMainTask);
	m_listeners.clear();
}

void CBlackMesaBotBehavior::Update()
{
	m_manager->Update(GetBot<CBlackMesaBot>());
}

std::vector<IEventListener*>* CBlackMesaBotBehavior::GetListenerVector()
{
	if (!m_manager)
	{
		return nullptr;
	}

	m_listeners.clear();
	m_listeners.push_back(m_manager.get());

	return &m_listeners;
}

IDecisionQuery* CBlackMesaBotBehavior::GetDecisionQueryResponder()
{
	return m_manager.get();
}
