#ifndef NAVBOT_BASEBOT_BEHAVIOR_H_
#define NAVBOT_BASEBOT_BEHAVIOR_H_

#include <memory>
#include <bot/interfaces/behavior.h>

class CBaseBot;

class CBaseBotBehavior : public IBehavior
{
public:
	CBaseBotBehavior(CBaseBot* bot);
	~CBaseBotBehavior() override;

	void Reset() override;
	void Update() override;

	IDecisionQuery* GetDecisionQueryResponder() override { return m_manager.get(); }
	std::vector<IEventListener*>* GetListenerVector() override;

private:
	std::unique_ptr<AITaskManager<CBaseBot>> m_manager;
	std::vector<IEventListener*> m_listeners;
};

#endif // !NAVBOT_BASEBOT_BEHAVIOR_H_
