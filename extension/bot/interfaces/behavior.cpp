#include "behavior.h"

IBehavior::IBehavior(CBaseBot* bot) : IBotInterface(bot)
{
}

IBehavior::~IBehavior()
{
}

void IBehavior::Reset()
{
}

void IBehavior::Update()
{
}

void IBehavior::Frame()
{
}

QueryAnswerType IBehavior::ShouldAttack(CBaseBot* me, const CKnownEntity* them)
{
	return GetDecisionQueryResponder()->ShouldAttack(me, them);
}

QueryAnswerType IBehavior::ShouldSeekAndDestroy(CBaseBot* me, const CKnownEntity* them)
{
	return GetDecisionQueryResponder()->ShouldSeekAndDestroy(me, them);
}

QueryAnswerType IBehavior::ShouldPickup(CBaseBot* me, edict_t* item)
{
	return GetDecisionQueryResponder()->ShouldPickup(me, item);
}

QueryAnswerType IBehavior::ShouldHurry(CBaseBot* me)
{
	return GetDecisionQueryResponder()->ShouldHurry(me);
}

QueryAnswerType IBehavior::ShouldRetreat(CBaseBot* me)
{
	return GetDecisionQueryResponder()->ShouldRetreat(me);
}

QueryAnswerType IBehavior::ShouldUse(CBaseBot* me, edict_t* object)
{
	return GetDecisionQueryResponder()->ShouldUse(me, object);
}

QueryAnswerType IBehavior::ShouldFreeRoam(CBaseBot* me)
{
	return GetDecisionQueryResponder()->ShouldFreeRoam(me);
}

QueryAnswerType IBehavior::IsBlocker(CBaseBot* me, edict_t* blocker, const bool any)
{
	return GetDecisionQueryResponder()->IsBlocker(me, blocker, any);
}

const CKnownEntity* IBehavior::SelectTargetThreat(CBaseBot* me, const CKnownEntity* threat1, const CKnownEntity* threat2)
{
	return GetDecisionQueryResponder()->SelectTargetThreat(me, threat1, threat2);
}

Vector IBehavior::GetTargetAimPos(CBaseBot* me, edict_t* entity, CBaseExtPlayer* player)
{
	return GetDecisionQueryResponder()->GetTargetAimPos(me, entity, player);
}
