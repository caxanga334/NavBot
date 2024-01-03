#ifndef SMNAV_BOT_DECISION_QUERY_H_
#define SMNAV_BOT_DECISION_QUERY_H_

struct edict_t;
class CBaseBot;
class CKnownEntity;
class CBaseExtPlayer;
class Vector;

#include "mathlib.h"

#undef max
#undef min
#undef clamp // undef valve mathlib crap

// An answer that can be given to a specific decision query
enum QueryAnswerType
{
	ANSWER_NO = 0,
	ANSWER_YES,
	ANSWER_UNDEFINED,

	MAX_ANSWER_TYPES
};

// Interface for querying decisions thay are depedant on the current bot AI state
class IDecisionQuery
{
public:
	virtual ~IDecisionQuery() {}

	// Should the bot attack this threat?
	virtual QueryAnswerType ShouldAttack(const CBaseBot* me, CKnownEntity* them);
	// Should the bot seek and destroy this threat?
	virtual QueryAnswerType ShouldSeekAndDestroy(const CBaseBot* me, CKnownEntity* them);
	// Should the bot pick up this item/weapon?
	virtual QueryAnswerType ShouldPickup(const CBaseBot* me, edict_t* item);
	// Should the bot be in a hurry?
	virtual QueryAnswerType ShouldHurry(const CBaseBot* me);
	// Should the bot retreat?
	virtual QueryAnswerType ShouldRetreat(const CBaseBot* me);
	// Should the bot use/operate this object (a button, door, lever, elevator, ...)
	virtual QueryAnswerType ShouldUse(const CBaseBot* me, edict_t* object);
	// Should the bot ignore objetives and walk around the map
	virtual QueryAnswerType ShouldFreeRoam(const CBaseBot* me);
	/**
	 * @brief Is the given entity a blocker that the bot should wait/avoid?
	 * @param me Bot itself
	 * @param blocker Entity blocking the bot's path
	 * @param any if true, we're testing to see if ANYTHING can be considerated a blocker
	 * @return Query Answer
	*/
	virtual QueryAnswerType IsBlocker(const CBaseBot* me, edict_t* blocker, const bool any = false);
	// Given two known entities, select which one the bot should target first
	virtual CKnownEntity* SelectTargetThreat(const CBaseBot* me, CKnownEntity* threat1, CKnownEntity* threat2);
	// Given a entity, returns a vector of where the bot should aim at. The player parameter may be NULL.
	virtual Vector GetTargetAimPos(const CBaseBot* me, edict_t* entity, CBaseExtPlayer* player = nullptr);
private:

};

inline QueryAnswerType IDecisionQuery::ShouldAttack(const CBaseBot* me, CKnownEntity* them)
{
	return ANSWER_UNDEFINED;
}

inline QueryAnswerType IDecisionQuery::ShouldSeekAndDestroy(const CBaseBot* me, CKnownEntity* them)
{
	return ANSWER_UNDEFINED;
}

inline QueryAnswerType IDecisionQuery::ShouldPickup(const CBaseBot* me, edict_t* item)
{
	return ANSWER_UNDEFINED;
}

inline QueryAnswerType IDecisionQuery::ShouldHurry(const CBaseBot* me)
{
	return ANSWER_UNDEFINED;
}

inline QueryAnswerType IDecisionQuery::ShouldRetreat(const CBaseBot* me)
{
	return ANSWER_UNDEFINED;
}

inline QueryAnswerType IDecisionQuery::ShouldUse(const CBaseBot* me, edict_t* object)
{
	return ANSWER_UNDEFINED;
}

inline QueryAnswerType IDecisionQuery::ShouldFreeRoam(const CBaseBot* me)
{
	return ANSWER_UNDEFINED;
}

inline QueryAnswerType IDecisionQuery::IsBlocker(const CBaseBot* me, edict_t* blocker, const bool any)
{
	return ANSWER_UNDEFINED;
}

inline CKnownEntity* IDecisionQuery::SelectTargetThreat(const CBaseBot* me, CKnownEntity* threat1, CKnownEntity* threat2)
{
	return nullptr;
}

inline Vector IDecisionQuery::GetTargetAimPos(const CBaseBot* me, edict_t* entity, CBaseExtPlayer* player)
{
	return vec3_origin;
}

#endif // !SMNAV_BOT_DECISION_QUERY_H_

