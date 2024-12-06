#ifndef NAVBOT_PLUGIN_BOT_BEHAVIOR_H_
#define NAVBOT_PLUGIN_BOT_BEHAVIOR_H_

#include <vector>
#include <memory>

#include <bot/interfaces/behavior.h>

class CPluginBot;

class CPluginBotBehavior : public IBehavior
{
public:
	CPluginBotBehavior(CBaseBot* bot);
	~CPluginBotBehavior() override;

	void Reset() override;
	void Update() override;

	std::vector<IEventListener*>* GetListenerVector() override;
	IDecisionQuery* GetDecisionQueryResponder() override;

private:
	std::unique_ptr<AITaskManager<CPluginBot>> m_manager;
	std::vector<IEventListener*> m_listeners;
};

#endif // !NAVBOT_PLUGIN_BOT_BEHAVIOR_H_
