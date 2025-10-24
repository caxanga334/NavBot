#include NAVBOT_PCH_FILE
#include <extension.h>
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

QueryAnswerType IBehavior::ShouldPickup(CBaseBot* me, CBaseEntity* item)
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

QueryAnswerType IBehavior::IsIgnoringMapObjectives(CBaseBot* me)
{
	return GetDecisionQueryResponder()->IsIgnoringMapObjectives(me);
}

QueryAnswerType IBehavior::IsBlocker(CBaseBot* me, CBaseEntity* blocker, const bool any)
{
	return GetDecisionQueryResponder()->IsBlocker(me, blocker, any);
}

const CKnownEntity* IBehavior::SelectTargetThreat(CBaseBot* me, const CKnownEntity* threat1, const CKnownEntity* threat2)
{
	return GetDecisionQueryResponder()->SelectTargetThreat(me, threat1, threat2);
}

Vector IBehavior::GetTargetAimPos(CBaseBot* me, CBaseEntity* entity, DesiredAimSpot desiredAim)
{
	return GetDecisionQueryResponder()->GetTargetAimPos(me, entity, desiredAim);
}

QueryAnswerType IBehavior::IsReady(CBaseBot* me)
{
	return GetDecisionQueryResponder()->IsReady(me);
}

QueryAnswerType IBehavior::ShouldAssistTeammate(CBaseBot* me, CBaseEntity* teammate)
{
	return GetDecisionQueryResponder()->ShouldAssistTeammate(me, teammate);
}

QueryAnswerType IBehavior::ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon)
{
	return GetDecisionQueryResponder()->ShouldSwitchToWeapon(me, weapon);
}
