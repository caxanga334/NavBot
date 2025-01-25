#ifndef NAVBOT_BLACK_MESA_BOT_BEHAVIOR_H_
#define NAVBOT_BLACK_MESA_BOT_BEHAVIOR_H_
#pragma once

#include <vector>
#include <memory>

#include <bot/interfaces/behavior.h>

class CBlackMesaBot;

class CBlackMesaBotBehavior : public IBehavior
{
public:
	CBlackMesaBotBehavior(CBaseBot* bot);
	~CBlackMesaBotBehavior() override;

	void Reset() override;
	void Update() override;

	std::vector<IEventListener*>* GetListenerVector() override;
	IDecisionQuery* GetDecisionQueryResponder() override;
private:
	std::unique_ptr<AITaskManager<CBlackMesaBot>> m_manager;
	std::vector<IEventListener*> m_listeners;
};

#endif // !NAVBOT_BLACK_MESA_BOT_BEHAVIOR_H_