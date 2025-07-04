#include NAVBOT_PCH_FILE
#include <extension.h>
#include "pluginbot.h"
#include "pluginbot_behavior.h"

class CPluginBotEmptyTask : public AITask<CPluginBot>
{
public:

	TaskResult<CPluginBot> OnTaskUpdate(CPluginBot* bot) override { return Continue(); }

	const char* GetName() const override { return "PluginBot"; }
};

CPluginBotBehavior::CPluginBotBehavior(CBaseBot* bot) : IBehavior(bot)
{
	m_manager = std::make_unique<AITaskManager<CPluginBot>>(new CPluginBotEmptyTask);
}

CPluginBotBehavior::~CPluginBotBehavior()
{
}

void CPluginBotBehavior::Reset()
{
	m_listeners.clear();
	m_manager = nullptr;
	m_manager = std::make_unique<AITaskManager<CPluginBot>>(new CPluginBotEmptyTask);
	m_listeners.push_back(m_manager.get());
}

void CPluginBotBehavior::Update()
{
	m_manager->Update(static_cast<CPluginBot*>(GetBot()));
}

std::vector<IEventListener*>* CPluginBotBehavior::GetListenerVector()
{
	m_listeners.clear();

	m_listeners.push_back(m_manager.get());

	return &m_listeners;
}

IDecisionQuery* CPluginBotBehavior::GetDecisionQueryResponder()
{
	return m_manager.get();
}
