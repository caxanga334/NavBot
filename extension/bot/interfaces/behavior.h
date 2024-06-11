#ifndef SMNAV_BOT_BEHAVIOR_H_
#define SMNAV_BOT_BEHAVIOR_H_
#pragma once

#include "tasks.h"
#include "base_interface.h"
#include "decisionquery.h"

// Interface responsible for the bot's behavior
class IBehavior : public IBotInterface, public IDecisionQuery
{
public:
	IBehavior(CBaseBot* bot);
	virtual ~IBehavior();

	// Reset the interface to it's initial state
	virtual void Reset();
	// Called at intervals
	virtual void Update();
	// Called every server frame
	virtual void Frame();

	// Returns who will answer to decision queries
	virtual IDecisionQuery* GetDecisionQueryResponder() = 0;

	QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them) override;
	QueryAnswerType ShouldSeekAndDestroy(CBaseBot* me, const CKnownEntity* them) override;
	QueryAnswerType ShouldPickup(CBaseBot* me, edict_t* item) override;
	QueryAnswerType ShouldHurry(CBaseBot* me) override;
	QueryAnswerType ShouldRetreat(CBaseBot* me) override;
	QueryAnswerType ShouldUse(CBaseBot* me, edict_t* object) override;
	QueryAnswerType ShouldFreeRoam(CBaseBot* me) override;
	QueryAnswerType IsBlocker(CBaseBot* me, edict_t* blocker, const bool any = false) override;
	const CKnownEntity* SelectTargetThreat(CBaseBot* me, const CKnownEntity* threat1, const CKnownEntity* threat2) override;
	Vector GetTargetAimPos(CBaseBot* me, edict_t* entity, CBaseExtPlayer* player = nullptr) override;
	QueryAnswerType IsReady(CBaseBot* me) override;

private:

};

#endif // !SMNAV_BOT_BEHAVIOR_H_

