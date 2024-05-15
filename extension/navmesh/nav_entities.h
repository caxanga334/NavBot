//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// nav_entities.h
// Navigation entities
// Author: Michael S. Booth (mike@turtlerockstudios.com), January 2003

#ifndef NAV_ENTITIES_H
#define NAV_ENTITIES_H

#include "nav_mesh.h"
#include <sdkports/sdk_ehandle.h>
#include <utllinkedlist.h>
#include <fmtstr.h>
#include <string_t.h>

class NavEntity {
public:
	NavEntity(edict_t* pEnt) : pEnt(pEnt) {
	}

	virtual ~NavEntity() {
	}

	virtual void InputEnable() = 0;

	virtual void InputDisable() = 0;

	edict_t* getEntity() const {
		return pEnt;
	}

	virtual int DrawDebugTextOverlays( void );

protected:
	edict_t* pEnt;
};
//-----------------------------------------------------------------------------------------------------
/**
  * An entity that modifies pathfinding cost to all areas it overlaps, to allow map designers
  * to tell bots to avoid/prefer certain regions.
  */
class CFuncNavCost : public NavEntity
{
public:

	CFuncNavCost(edict_t *ent);
	virtual void UpdateOnRemove( void );

	void InputEnable();
	void InputDisable( );

	bool IsEnabled( void ) const { return !m_isDisabled; }

	void CostThink( CNavMesh* TheNavMesh );

	bool IsApplicableTo( edict_t *who ) const;			// Return true if this cost applies to the given actor

	virtual float GetCostMultiplier( edict_t *who ) const	{ return 1.0f; }

protected:

	int m_team;
	bool m_isDisabled;
	string_t m_iszTags;

	static CUtlVector< CHandle< CFuncNavCost > > gm_masterCostVector;
	static CountdownTimer gm_dirtyTimer;
	void UpdateAllNavCostDecoration( CNavMesh* TheNavMesh );


	// CUtlVector< CFmtStr > m_tags;
	bool HasTag( const char *groupname ) const;
};


//-----------------------------------------------------------------------------------------------------
class CFuncNavAvoid : public CFuncNavCost
{
public:

	virtual float GetCostMultiplier( edict_t *who ) const;		// return pathfind cost multiplier for the given actor
};


//-----------------------------------------------------------------------------------------------------
class CFuncNavPrefer : public CFuncNavCost
{
public:

	virtual float GetCostMultiplier( edict_t *who ) const;		// return pathfind cost multiplier for the given actor
};


//-----------------------------------------------------------------------------------------------------
/**
  * An entity that can block/unblock nav areas.  This is meant for semi-transient areas that block
  * pathfinding but can be ignored for longer-term queries like computing L4D flow distances and
  * escape routes.
  */
class CFuncNavBlocker : public NavEntity
{
public:
	CFuncNavBlocker(edict_t* pEnt);

	~CFuncNavBlocker() {
		UpdateOnRemove();
	}

	virtual void UpdateOnRemove( void );

	void InputEnable() {
		BlockNav();
	}

	void InputDisable() {
		UnblockNav();
	}

	void setBlockedTeam(int team) {
		m_blockedTeamNumber = team;
	}

	bool IsBlockingNav( int teamNumber ) const;

	int DrawDebugTextOverlays( void );

	bool operator()( CNavArea *area) const;	// functor that blocks areas in our extent

	static bool CalculateBlocked( bool *pResultByTeam, const Vector &vecMins, const Vector &vecMaxs );

private:
	void UpdateBlocked();

	static CUtlLinkedList<CFuncNavBlocker *> gm_NavBlockers;

	void BlockNav( void );
	void UnblockNav( void );
	void setBlockedTeam(bool block);
	bool m_isBlockingNav[2];
	int m_blockedTeamNumber;
	bool m_bDisabled;
	Vector m_CachedMins, m_CachedMaxs;

};

//-----------------------------------------------------------------------------------------------------
/**
  * An entity that can obstruct nav areas.  This is meant for semi-transient areas that obstruct
  * pathfinding but can be ignored for longer-term queries like computing L4D flow distances and
  * escape routes.
  */
class CFuncNavObstruction : public INavAvoidanceObstacle, public NavEntity
{

public:
	CFuncNavObstruction(edict_t* pEnt);

	virtual ~CFuncNavObstruction() {
		UpdateOnRemove();
	}

	virtual void UpdateOnRemove(void);

	void InputEnable();
	void InputDisable();

	virtual bool IsPotentiallyAbleToObstructNavAreas(void) const { return true; }		// could we at some future time obstruct nav?
	virtual float GetNavObstructionHeight(void) const;	// height at which to obstruct nav areas
	virtual bool CanObstructNavAreas(void) const { return !m_bDisabled; }				// can we obstruct nav right this instant?
	virtual edict_t *GetObstructingEntity(void) { return this->pEnt; }
	virtual void OnNavMeshLoaded(void)
	{
		if (!m_bDisabled)
		{
			ObstructNavAreas();
		}
	}

	int DrawDebugTextOverlays(void);

	bool operator()(CNavArea *area);	// functor that obstructs areas in our extent

private:
	void ObstructNavAreas(void);
	bool m_bDisabled;
};


#endif // NAV_ENTITIES_H
