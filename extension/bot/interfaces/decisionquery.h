#ifndef SMNAV_BOT_DECISION_QUERY_H_
#define SMNAV_BOT_DECISION_QUERY_H_

struct edict_t;
class CBaseBot;
class CKnownEntity;
class CBaseExtPlayer;
class CBotWeapon;
class CBaseEntity;

#include "mathlib.h"
#include <memory>

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
	virtual QueryAnswerType ShouldAttack(CBaseBot* me, const CKnownEntity* them);
	// Should the bot seek and destroy this threat?
	virtual QueryAnswerType ShouldSeekAndDestroy(CBaseBot* me, const CKnownEntity* them);
	// Should the bot pick up this item/weapon?
	virtual QueryAnswerType ShouldPickup(CBaseBot* me, CBaseEntity* item);
	// Should the bot be in a hurry?
	virtual QueryAnswerType ShouldHurry(CBaseBot* me);
	// Should the bot retreat?
	virtual QueryAnswerType ShouldRetreat(CBaseBot* me);
	// Is the bot ignoring the current map's objectives
	virtual QueryAnswerType IsIgnoringMapObjectives(CBaseBot* me);
	/**
	 * @brief Is the given entity a blocker that the bot should wait/avoid?
	 * @param me Bot itself
	 * @param blocker Entity blocking the bot's path
	 * @param any if true, we're testing to see if ANYTHING can be considerated a blocker
	 * @return Query Answer
	*/
	virtual QueryAnswerType IsBlocker(CBaseBot* me, CBaseEntity* blocker, const bool any = false);
	// Given two known entities, select which one the bot should target first
	virtual const CKnownEntity* SelectTargetThreat(CBaseBot* me, const CKnownEntity* threat1, const CKnownEntity* threat2);

	/**
	 * @brief Desired aim spot when aiming weapons at enemies
	 */
	enum DesiredAimSpot : int
	{
		AIMSPOT_NONE = 0, // No preference
		AIMSPOT_ABSORIGIN, // Prefer aiming at abs origin
		AIMSPOT_CENTER, // Prefer aiming at World Space Center
		AIMSPOT_HEAD, // Prefer aiming at the head/eye origin
		AIMSPOT_OFFSET, // Aim at abs origin + given offset (stored at IPlayerController)
		AIMSPOT_BONE, // Aim at a specific bone (stoed at IPlayerController)

		MAX_DESIRED_AIM_SPOTS
	};

	/**
	 * @brief Selects the position the bot should aim at a given entity.
	 * @param me Bot.
	 * @param entity Entity to aim at.
	 * @param desiredAim Desired aim spot.
	 * @return Position the bot should aim at, return vec3_origin for no answer.
	 */
	virtual Vector GetTargetAimPos(CBaseBot* me, CBaseEntity* entity, DesiredAimSpot desiredAim = AIMSPOT_NONE);
	// If a game mode has a toggle ready feature, this asks if the bot is ready
	virtual QueryAnswerType IsReady(CBaseBot* me);
	// Should the bot help a specific teammate?
	virtual QueryAnswerType ShouldAssistTeammate(CBaseBot* me, CBaseEntity* teammate);
	// Should the bot switch to this weapon?
	virtual QueryAnswerType ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon);
private:

};

inline QueryAnswerType IDecisionQuery::ShouldAttack(CBaseBot* me, const CKnownEntity* them)
{
	return ANSWER_UNDEFINED;
}

inline QueryAnswerType IDecisionQuery::ShouldSeekAndDestroy(CBaseBot* me, const CKnownEntity* them)
{
	return ANSWER_UNDEFINED;
}

inline QueryAnswerType IDecisionQuery::ShouldPickup(CBaseBot* me, CBaseEntity* item)
{
	return ANSWER_UNDEFINED;
}

inline QueryAnswerType IDecisionQuery::ShouldHurry(CBaseBot* me)
{
	return ANSWER_UNDEFINED;
}

inline QueryAnswerType IDecisionQuery::ShouldRetreat(CBaseBot* me)
{
	return ANSWER_UNDEFINED;
}

inline QueryAnswerType IDecisionQuery::IsIgnoringMapObjectives(CBaseBot* me)
{
	return ANSWER_UNDEFINED;
}

inline QueryAnswerType IDecisionQuery::IsBlocker(CBaseBot* me, CBaseEntity* blocker, const bool any)
{
	return ANSWER_UNDEFINED;
}

inline const CKnownEntity* IDecisionQuery::SelectTargetThreat(CBaseBot* me, const CKnownEntity* threat1, const CKnownEntity* threat2)
{
	return nullptr;
}

inline Vector IDecisionQuery::GetTargetAimPos(CBaseBot* me, CBaseEntity* entity, DesiredAimSpot desiredAim)
{
	return vec3_origin;
}

inline QueryAnswerType IDecisionQuery::IsReady(CBaseBot* me)
{
	return ANSWER_UNDEFINED;
}

inline QueryAnswerType IDecisionQuery::ShouldAssistTeammate(CBaseBot* me, CBaseEntity* teammate)
{
	return ANSWER_UNDEFINED;
}

inline QueryAnswerType IDecisionQuery::ShouldSwitchToWeapon(CBaseBot* me, const CBotWeapon* weapon)
{
	return ANSWER_UNDEFINED;
}

#endif // !SMNAV_BOT_DECISION_QUERY_H_

