#ifndef NAVBOT_TF2BOT_BEHAVIOR_H_
#define NAVBOT_TF2BOT_BEHAVIOR_H_
#pragma once

#include <vector>

#include <bot/interfaces/behavior.h>

class CTF2Bot;

class CTF2BotBehavior : public IBehavior
{
public:
	CTF2BotBehavior(CBaseBot* bot);
	~CTF2BotBehavior() override;

	void Reset() override;
	void Update() override;

	std::vector<IEventListener*>* GetListenerVector() override;
	IDecisionQuery* GetDecisionQueryResponder() override;
private:
	AITaskManager<CTF2Bot>* m_manager;
	std::vector<IEventListener*> m_listeners;
};

#endif // !NAVBOT_TF2BOT_BEHAVIOR_H_
