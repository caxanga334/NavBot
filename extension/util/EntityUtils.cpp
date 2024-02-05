/*
 * EntityUtils.cpp
 *
 *  Created on: Apr 15, 2017
 */

#include <string>

#include <extension.h>
#include <manager.h>
#include <mods/basemod.h>

#include "EntityUtils.h"

#include "BasePlayer.h"
#include <eiface.h>
#include <iplayerinfo.h>
#include <worldsize.h>

extern IVEngineServer *engine;
extern CGlobalVars *gpGlobals;

constexpr int PATTERN_DIRECTORY = 0x00000001;

static bool StringMatchesPattern(const char* source, const char* pattern, int nFlags)
{
	const char* pszSource = source;
	const char* pszPattern = pattern;
	bool bExact = true;

	while (1)
	{
		if ((*pszPattern) == 0)
		{
			return ((*pszSource) == 0);
		}

		if ((*pszPattern) == '*')
		{
			pszPattern++;

			if ((*pszPattern) == 0)
			{
				return true;
			}

			bExact = false;
			continue;
		}

		int nLength = 0;

		while ((*pszPattern) != '*' && (*pszPattern) != 0)
		{
			nLength++;
			pszPattern++;
		}

		while (1)
		{
			const char* pszStartPattern = pszPattern - nLength;
			const char* pszSearch = pszSource;

			for (int i = 0; i < nLength; i++, pszSearch++, pszStartPattern++)
			{
				if ((*pszSearch) == 0)
				{
					return false;
				}

				if ((*pszSearch) != (*pszStartPattern))
				{
					break;
				}
			}

			if (pszSearch - pszSource == nLength)
			{
				break;
			}

			if (bExact == true)
			{
				return false;
			}

			if ((nFlags & PATTERN_DIRECTORY) != 0)
			{
				if ((*pszPattern) != '/' && (*pszSource) == '/')
				{
					return false;
				}
			}

			pszSource++;
		}

		pszSource += nLength;
	}
}

static bool StringMatchesPattern(const std::string& source, const std::string& pattern, int nFlags)
{
	const char* pszSource = source.c_str();
	const char* pszPattern = pattern.c_str();
	bool bExact = true;

	while (1)
	{
		if ((*pszPattern) == 0)
		{
			return ((*pszSource) == 0);
		}

		if ((*pszPattern) == '*')
		{
			pszPattern++;

			if ((*pszPattern) == 0)
			{
				return true;
			}

			bExact = false;
			continue;
		}

		int nLength = 0;

		while ((*pszPattern) != '*' && (*pszPattern) != 0)
		{
			nLength++;
			pszPattern++;
		}

		while (1)
		{
			const char* pszStartPattern = pszPattern - nLength;
			const char* pszSearch = pszSource;

			for (int i = 0; i < nLength; i++, pszSearch++, pszStartPattern++)
			{
				if ((*pszSearch) == 0)
				{
					return false;
				}

				if ((*pszSearch) != (*pszStartPattern))
				{
					break;
				}
			}

			if (pszSearch - pszSource == nLength)
			{
				break;
			}

			if (bExact == true)
			{
				return false;
			}

			if ((nFlags & PATTERN_DIRECTORY) != 0)
			{
				if ((*pszPattern) != '/' && (*pszSource) == '/')
				{
					return false;
				}
			}

			pszSource++;
		}

		pszSource += nLength;
	}
}

template<typename Func>
void forAllEntities(const Func& func,
		int startIndex = gpGlobals->maxClients + 1) {
	for (int i = startIndex; i < gpGlobals->maxEntities; i++) {
		edict_t* ent = gamehelpers->EdictOfIndex(i);
		if (ent != nullptr && !ent->IsFree()) {
			func(ent);
		}
	}
}

template<typename Func>
void findEntWithName(const char* name, const Func& match,
		CUtlLinkedList<edict_t*>& result) {
	forAllEntities([match, name, &result](edict_t* ent) -> void {
		if (!ent->IsFree() && ent->GetNetworkable() != nullptr
				&& (name == ent->GetClassName() || match(name, ent->GetClassName()))) {
			result.AddToTail(ent);
		}
	});
}

void findEntWithMatchingName(const char* name,
		CUtlLinkedList<edict_t*>& result) {
	findEntWithName(name,
			[name, &result] (const char* targetName, const char* className) -> bool {
				return Q_strcmp(className, targetName) == 0;
			}, result);
}

void findEntWithPatternInName(const char* name,
		CUtlLinkedList<edict_t*>& result) {
	findEntWithName(name,
			[name, &result] (const char* targetName, const char* className) -> bool {
				return StringMatchesPattern(className, targetName, 0);
			}, result);
}

edict_t * findNearestEntity(const CUtlLinkedList<edict_t*>& ent,
		const Vector& pos, float maxRadius) {
	float mindist = pow(maxRadius, 2);
	edict_t* ret = nullptr;
	if (mindist == 0) {
		mindist = MAX_TRACE_LENGTH * MAX_TRACE_LENGTH;
	}
	FOR_EACH_LL(ent, i)
	{
		if (ent[i]->IsFree()) {
			continue;
		}
		float dist = (pos - ent[i]->GetIServerEntity()->GetCollideable()->GetCollisionOrigin()).LengthSqr();
		if (dist < mindist) {
			mindist = dist;
			ret = ent[i];
		}
	}
	return ret;
}

// MatchesPattern is exclusive to SDK 2013
//bool FClassnameIs(edict_t *pEntity, const char *szClassname) {
//	Assert(pEntity);
//	castable_string_t className(pEntity->GetClassName());
//	return IDENT_STRINGS(className, szClassname)
//			|| CUtlString(className.ToCStr()).MatchesPattern(szClassname);
//}

bool FClassnameIs(edict_t* pEntity, const char* szClassname) {
	auto classname = pEntity->GetClassName();

	if (Q_strcmp(classname, szClassname) == 0) { return true; }

	std::string s1(classname);
	std::string s2(szClassname);

	return StringMatchesPattern(s1, s2, 0);
}

bool FClassnameIs(CBaseEntity* pEntity, const char* szClassname)
{
	auto classname = gamehelpers->GetEntityClassname(pEntity);

	if (Q_strcmp(classname, szClassname) == 0) { return true; }

	std::string s1(classname);
	std::string s2(szClassname);

	return StringMatchesPattern(s1, s2, 0);
}

bool isBreakable(edict_t* target) {
	BaseEntity obj(target);
	const char* model = target->GetIServerEntity()->GetModelName().ToCStr();
	return !(*obj.getFlags() & FL_WORLDBRUSH)
			&& (model[13] != 'c' || model[17] != 'o' || model[20] != 'd'
					|| model[28] != 'e') // explosive
			&& *obj.getHealth() < 1000 && *obj.getHealth() > 0;
}

/**
 * Return true if given entity can be ignored when moving
 */
bool IsEntityWalkable(edict_t *entity, unsigned int flags) {
	extern ConVar sm_nav_solid_props;

	if (FClassnameIs(entity, "worldspawn") || FClassnameIs(entity, "player"))
		return false;

	// if we hit a door, assume its walkable because it will open when we touch it
	if (FClassnameIs(entity, "func_door*")) {
#ifdef PROBLEMATIC	// cp_dustbowl doors dont open by touch - they use surrounding triggers
		if ( !entity->HasSpawnFlags( SF_DOOR_PTOUCH ) )
		{
			// this door is not opened by touching it, if it is closed, the area is blocked
			CBaseDoor *door = (CBaseDoor *)entity;
			return door->m_toggle_state == TS_AT_TOP;
		}
#endif // _DEBUG

		return (flags & WALK_THRU_FUNC_DOORS);
	}

	if (FClassnameIs(entity, "prop_door*")) {
		return (flags & WALK_THRU_PROP_DOORS);
	}

	// if we hit a clip brush, ignore it if it is not BRUSHSOLID_ALWAYS
	if (FClassnameIs(entity, "func_brush")) {
		switch ( entity->GetCollideable()->GetSolidFlags( ))
				{
				case BRUSHSOLID_ALWAYS:
					return false;
				case BRUSHSOLID_NEVER:
					return true;
				case BRUSHSOLID_TOGGLE:
					return (flags & WALK_THRU_TOGGLE_BRUSHES) != 0;
				}
	}

#if SOURCE_ENGINE == SE_TF2
	if (FClassnameIs(entity, "func_respawnroomvisualizer"))
	{
		return true;
	}
#endif // SOURCE_ENGINE == SE_TF2

	auto mod = extmanager->GetMod();

	if (mod && !mod->NavIsEntityIgnored(entity, flags))
	{
		return mod->NavIsEntityWalkable(entity, flags);
	}

	// if we hit a breakable object, assume its walkable because we will shoot it when we touch it
	return (((FClassnameIs( entity, "func_breakable" ) || FClassnameIs( entity, "func_breakable_surf" ))
			&& *BaseEntity(entity).getHealth() > 0) && (flags & WALK_THRU_BREAKABLES))
			|| FClassnameIs( entity, "func_playerinfected_clip" )
			|| (sm_nav_solid_props.GetBool() && FClassnameIs( entity, "prop_*" ));
}

edict_t* UTIL_GetListenServerEnt() {
	// no "local player" if this is a dedicated server or a single player game
	if (engine->IsDedicatedServer()) {
		Warning("UTIL_GetListenServerHost() called from a dedicated server or single-player game.\n");
		return nullptr;
	}

	return gamehelpers->EdictOfIndex(1);
}

IPlayerInfo *UTIL_GetListenServerHost(void) {
	edict_t *ent = UTIL_GetListenServerEnt();
	extern IPlayerInfoManager* playerinfomanager;
	return ent == nullptr ? nullptr : playerinfomanager->GetPlayerInfo(ent);
}
