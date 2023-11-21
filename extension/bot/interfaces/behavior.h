#ifndef SMNAV_BOT_BEHAVIOR_H_
#define SMNAV_BOT_BEHAVIOR_H_

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

	virtual QueryAnswerType ShouldAttack(const CBaseBot* me, CKnownEntity* them) override;
	virtual QueryAnswerType ShouldPickup(const CBaseBot* me, edict_t* item) override;
	virtual QueryAnswerType ShouldHurry(const CBaseBot* me) override;
	virtual QueryAnswerType ShouldRetreat(const CBaseBot* me);
	virtual QueryAnswerType ShouldUse(const CBaseBot* me, edict_t* object) override;
	virtual QueryAnswerType ShouldFreeRoam(const CBaseBot* me) override;
	virtual CKnownEntity* SelectTargetThreat(const CBaseBot* me, CKnownEntity* threat1, CKnownEntity* threat2) override;
	virtual Vector GetTargetAimPos(const CBaseBot* me, edict_t* entity, CBaseExtPlayer* player = nullptr) override;

private:

};

#endif // !SMNAV_BOT_BEHAVIOR_H_

