#include <extension.h>
#include <bot/interfaces/tasks.h>
#include <bot/tf2/tasks/tf2bot_maintask.h>
#include <bot/tf2/tf2bot.h>
#include "tf2bot_behavior.h"

CTF2BotBehavior::CTF2BotBehavior(CBaseBot* bot) : IBehavior(bot)
{
	m_manager = new AITaskManager<CTF2Bot>(new CTF2BotMainTask);
	m_listeners.reserve(2);
}

CTF2BotBehavior::~CTF2BotBehavior()
{
	m_listeners.clear();
	delete m_manager;
	m_manager = nullptr;
}

void CTF2BotBehavior::Reset()
{
	delete m_manager;
	m_manager = new AITaskManager<CTF2Bot>(new CTF2BotMainTask);
	m_listeners.clear();
}

void CTF2BotBehavior::Update()
{
	m_manager->Update(static_cast<CTF2Bot*>(GetBot()));
}

std::vector<IEventListener*>* CTF2BotBehavior::GetListenerVector()
{
	if (m_manager == nullptr)
	{
		return nullptr;
	}

	m_listeners.clear();
	m_listeners.push_back(m_manager);

	return &m_listeners;
}

IDecisionQuery* CTF2BotBehavior::GetDecisionQueryResponder()
{
	return m_manager;
}
