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
	QueryAnswerType ShouldPickup(CBaseBot* me, edict_t* item) override;
	QueryAnswerType ShouldHurry(CBaseBot* me) override;
	QueryAnswerType ShouldRetreat(CBaseBot* me) override;
	QueryAnswerType ShouldUse(CBaseBot* me, edict_t* object) override;
	QueryAnswerType ShouldFreeRoam(CBaseBot* me) override;
	QueryAnswerType IsBlocker(CBaseBot* me, edict_t* blocker, const bool any = false) override;
	std::shared_ptr<const CKnownEntity> SelectTargetThreat(CBaseBot* me, std::shared_ptr<const CKnownEntity> threat1, std::shared_ptr<const CKnownEntity> threat2) override;
	Vector GetTargetAimPos(CBaseBot* me, edict_t* entity, CBaseExtPlayer* player = nullptr) override;
	QueryAnswerType IsReady(CBaseBot* me) override;
	QueryAnswerType ShouldAssistTeammate(CBaseBot* me, CBaseExtPlayer& teammate) override;
	QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon) override;

private:

};

#endif // !SMNAV_BOT_BEHAVIOR_H_

