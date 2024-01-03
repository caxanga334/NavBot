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

QueryAnswerType IBehavior::ShouldAttack(const CBaseBot* me, CKnownEntity* them)
{
	return GetDecisionQueryResponder()->ShouldAttack(me, them);
}

QueryAnswerType IBehavior::ShouldSeekAndDestroy(const CBaseBot* me, CKnownEntity* them)
{
	return GetDecisionQueryResponder()->ShouldSeekAndDestroy(me, them);
}

QueryAnswerType IBehavior::ShouldPickup(const CBaseBot* me, edict_t* item)
{
	return GetDecisionQueryResponder()->ShouldPickup(me, item);
}

QueryAnswerType IBehavior::ShouldHurry(const CBaseBot* me)
{
	return GetDecisionQueryResponder()->ShouldHurry(me);
}

QueryAnswerType IBehavior::ShouldRetreat(const CBaseBot* me)
{
	return GetDecisionQueryResponder()->ShouldRetreat(me);
}

QueryAnswerType IBehavior::ShouldUse(const CBaseBot* me, edict_t* object)
{
	return GetDecisionQueryResponder()->ShouldUse(me, object);
}

QueryAnswerType IBehavior::ShouldFreeRoam(const CBaseBot* me)
{
	return GetDecisionQueryResponder()->ShouldFreeRoam(me);
}

QueryAnswerType IBehavior::IsBlocker(const CBaseBot* me, edict_t* blocker, const bool any)
{
	return GetDecisionQueryResponder()->IsBlocker(me, blocker, any);
}

CKnownEntity* IBehavior::SelectTargetThreat(const CBaseBot* me, CKnownEntity* threat1, CKnownEntity* threat2)
{
	return GetDecisionQueryResponder()->SelectTargetThreat(me, threat1, threat2);
}

Vector IBehavior::GetTargetAimPos(const CBaseBot* me, edict_t* entity, CBaseExtPlayer* player)
{
	return GetDecisionQueryResponder()->GetTargetAimPos(me, entity, player);
}
