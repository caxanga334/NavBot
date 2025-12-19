#ifndef SMNAV_BOT_BEHAVIOR_H_
#define SMNAV_BOT_BEHAVIOR_H_
#pragma once

#include <memory>
#include "tasks.h"
#include "base_interface.h"
#include "decisionquery.h"

// Interface responsible for the bot's behavior
class IBehavior : public IBotInterface, public IDecisionQuery
{
public:
	IBehavior(CBaseBot* bot);
	~IBehavior() override;

	// Reset the interface to it's initial state
	void Reset() override;
	// Called at intervals
	void Update() override;
	// Called every server frame
	void Frame() override;

	// Returns who will answer to decision queries
	virtual IDecisionQuery* GetDecisionQueryResponder() = 0;

	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override;
	QueryAnswerType ShouldSeekAndDestroy(CBaseBot* me, const CKnownEntity* them) override;
	QueryAnswerType ShouldPickup(CBaseBot* me, CBaseEntity* item) override;
	QueryAnswerType ShouldHurry(CBaseBot* me) override;
	QueryAnswerType ShouldRetreat(CBaseBot* me) override;
	QueryAnswerType IsIgnoringMapObjectives(CBaseBot* me) override;
	QueryAnswerType IsBlocker(CBaseBot* me, CBaseEntity* blocker, const bool any = false) override;
	const CKnownEntity* SelectTargetThreat(CBaseBot* me, const CKnownEntity* threat1, const CKnownEntity* threat2) override;
	Vector GetTargetAimPos(CBaseBot* me, CBaseEntity* entity, DesiredAimSpot desiredAim = AIMSPOT_NONE) override;
	QueryAnswerType IsReady(CBaseBot* me) override;
	QueryAnswerType ShouldAssistTeammate(CBaseBot* me, CBaseEntity* teammate) override;
	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override;
	bool IsBehaviorRunning(int id, int flags, bool ismod) override;

private:

};

template <class Bot, class Manager, class InitialTask>
class CDefaultBehaviorImplementation : public IBehavior
{
public:
	CDefaultBehaviorImplementation(Bot* bot) :
		IBehavior(bot)
	{
		m_manager = std::make_unique<Manager>(new InitialTask);
		m_listeners.reserve(2);
		m_listeners.push_back(m_manager.get());
	}

	void Reset() override
	{
		m_manager = nullptr;
		m_manager = std::make_unique<Manager>(new InitialTask);
		m_listeners.clear();
		m_listeners.push_back(m_manager.get());
	}

	void Update() override
	{
		m_manager->Update(GetBot<Bot>());
	}

	IDecisionQuery* GetDecisionQueryResponder() override { return m_manager.get(); }

	std::vector<IEventListener*>* GetListenerVector() override
	{
		m_listeners.clear();
		m_listeners.push_back(m_manager.get());
		return &m_listeners;
	}

private:
	std::unique_ptr<Manager> m_manager;
	std::vector<IEventListener*> m_listeners;
};

#endif // !SMNAV_BOT_BEHAVIOR_H_

