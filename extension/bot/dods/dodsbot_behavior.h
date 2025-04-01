#ifndef __NAVBOT_DODSBOT_BEHAVIOR_H_
#define __NAVBOT_DODSBOT_BEHAVIOR_H_

#include <bot/interfaces/behavior.h>

class CDoDSBot;

class CDoDSBotBehavior : public IBehavior
{
public:
	CDoDSBotBehavior(CBaseBot* bot);
	~CDoDSBotBehavior() override;

	void Reset() override;
	void Update() override;

	std::vector<IEventListener*>* GetListenerVector() override;
	IDecisionQuery* GetDecisionQueryResponder() override;
private:
	AITaskManager<CDoDSBot>* m_manager;
	std::vector<IEventListener*> m_listeners;
};

#endif // !__NAVBOT_DODSBOT_BEHAVIOR_H_
