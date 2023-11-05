//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// nav_entities.cpp
// AI Navigation entities
// Author: Michael S. Booth (mike@turtlerockstudios.com), January 2003

#include "nav_entities.h"

#include "nav_area.h"
#include <util/UtilTrace.h>
#include <eiface.h>
#include <iplayerinfo.h>
#include <collisionutils.h>
#include <ivdebugoverlay.h>

#include "nav_macros.h"

// the global singleton interface
extern CNavMesh *TheNavMesh;
extern IPlayerInfoManager* playerinfomanager;
extern NavAreaVector TheNavAreas;
extern IVDebugOverlay* debugoverlay;

//--------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------

std::vector< CHandle< CFuncNavCost > > CFuncNavCost::gm_masterCostVector;
CountdownTimer CFuncNavCost::gm_dirtyTimer;

#define UPDATE_DIRTY_TIME 0.2f

//--------------------------------------------------------------------------------------------------------
CFuncNavCost::CFuncNavCost( edict_t* pEnt ): NavEntity(pEnt), m_isDisabled(true),
		m_team(0)
{
	gm_masterCostVector.push_back(this);
	gm_dirtyTimer.Start( UPDATE_DIRTY_TIME );
/**TODO
	SetSolid( SOLID_BSP );	
	AddSolidFlags( FSOLID_NOT_SOLID );

	SetMoveType( MOVETYPE_NONE );
	SetModel( STRING( GetModelName() ) );
	AddEffects( EF_NODRAW );
	SetCollisionGroup( COLLISION_GROUP_NONE );

	VPhysicsInitShadow( false, false );

	SetThink( &CFuncNavCost::CostThink );
	SetNextThink( playerinfomanager->GetGlobalVars()->curtime + UPDATE_DIRTY_TIME );
*/

	m_tags.clear();

	const char *tags = STRING( m_iszTags );

	// chop space-delimited string into individual tokens
	if ( tags )
	{
		// char *buffer = V_strdup ( tags );
		// Manual reimplementation of V_strdup since it's only available on SDK 2013

		auto size = std::strlen(tags);
		char* buffer = new char[size + 1];
		std::memcpy(buffer, tags, size + 1);

		for( char *token = strtok( buffer, " " ); token; token = strtok( NULL, " " ) )
		{
			m_tags.emplace_back(token);
			// m_tags.AddToTail( CFmtStr( "%s", token ) );
		}

		delete [] buffer;
	}
}


//--------------------------------------------------------------------------------------------------------
void CFuncNavCost::UpdateOnRemove( void )
{
	NAV_VEC_REMOVE_NO_DELETE(gm_masterCostVector, this);

	// gm_masterCostVector.FindAndFastRemove( this );

	gm_dirtyTimer.Start( UPDATE_DIRTY_TIME );
}


//--------------------------------------------------------------------------------------------------------
void CFuncNavCost::InputEnable( )
{
	m_isDisabled = false;
	gm_dirtyTimer.Start( UPDATE_DIRTY_TIME );
}


//--------------------------------------------------------------------------------------------------------
void CFuncNavCost::InputDisable( )
{
	m_isDisabled = true;
	gm_dirtyTimer.Start( UPDATE_DIRTY_TIME );
}


//--------------------------------------------------------------------------------------------------------
void CFuncNavCost::CostThink( CNavMesh* TheNavMesh )
{
	// TODO: SetNextThink( playerinfomanager->GetGlobalVars()->curtime + UPDATE_DIRTY_TIME );

	if ( gm_dirtyTimer.HasStarted() && gm_dirtyTimer.IsElapsed() )
	{
		// one or more avoid entities have changed - update nav decoration
		gm_dirtyTimer.Invalidate();

		UpdateAllNavCostDecoration(TheNavMesh);
	}
}


//--------------------------------------------------------------------------------------------------------
bool CFuncNavCost::HasTag( const char *groupname ) const
{
	for (auto& string : m_tags)
	{
		auto tag = string.c_str();

		if (strcmp(tag, groupname) == 0)
		{
			return true;
		}
	}

	return false;
}


//--------------------------------------------------------------------------------------------------------
// Return true if this cost applies to the given actor
bool CFuncNavCost::IsApplicableTo( edict_t *who ) const
{
	if ( !who || ( m_team > 0 && playerinfomanager->GetPlayerInfo(who)->GetTeamIndex() != m_team )) {
		return false;
	}

#ifdef TF_DLL
	// TODO: Make group comparison efficient and move to base combat character
	CTFBot *bot = ToTFBot( who );
	if ( bot )
	{
		if ( bot->HasTheFlag() )
		{
			if ( HasTag( "bomb_carrier" ) )
			{
				return true;
			}

			// check custom bomb_carrier tags for this bot
			for( int i=0; i<m_tags.Count(); ++i )
			{
				const char* pszTag = m_tags[i];
				if ( V_stristr( pszTag, "bomb_carrier" ) )
				{
					if ( bot->HasTag( pszTag ) )
					{
						return true;
					}
				}
			}

			// the bomb carrier only pays attention to bomb_carrier costs
			return false;
		}

		if ( bot->HasMission( CTFBot::MISSION_DESTROY_SENTRIES ) )
		{
			if ( HasTag( "mission_sentry_buster" ) )
			{
				return true;
			}
		}
		
		if ( bot->HasMission( CTFBot::MISSION_SNIPER ) )
		{
			if ( HasTag( "mission_sniper" ) )
			{
				return true;
			}
		}
		
		if ( bot->HasMission( CTFBot::MISSION_SPY ) )
		{
			if ( HasTag( "mission_spy" ) )
			{
				return true;
			}
		}

		if ( bot->HasMission( CTFBot::MISSION_REPROGRAMMED ) )
		{
			return false;
		}

		if ( !bot->IsOnAnyMission() )
		{
			if ( HasTag( "common" ) )
			{
				return true;
			}
		}

		if ( HasTag( bot->GetPlayerClass()->GetName() ) )
		{
			return true;
		}

		// check custom tags for this bot
		for( int i=0; i<m_tags.Count(); ++i )
		{
			if ( bot->HasTag( m_tags[i] ) )
			{
				return true;
			}
		}

		// this cost doesn't apply to me
		return false;
	}
#endif

	return false;
}


//--------------------------------------------------------------------------------------------------------
// Reevaluate all func_nav_cost entities and update the nav decoration accordingly.
// This is required to handle overlapping func_nav_cost entities.
void CFuncNavCost::UpdateAllNavCostDecoration( CNavMesh* TheNavMesh )
{
	// first, clear all avoid decoration from the mesh

	for (auto area : TheNavAreas)
	{
		area->ClearAllNavCostEntities();
	}

	for (auto& funccost : gm_masterCostVector)
	{
		CFuncNavCost* cost = funccost.Get();

		if (!cost || !cost->IsEnabled())
		{
			continue;
		}

		Extent extent;
		extent.Init(pEnt);

		std::vector< CNavArea* > overlapVector;
		TheNavMesh->CollectAreasOverlappingExtent(extent, &overlapVector);

		Ray_t ray;
		trace_t tr;
		for (size_t j = 0; j < overlapVector.size(); ++j)
		{
			ray.Init(overlapVector[j]->GetCenter(), overlapVector[j]->GetCenter());
			extern IEngineTrace* enginetrace;
			enginetrace->ClipRayToCollideable(ray, MASK_ALL, pEnt->GetCollideable(), &tr);

			if (tr.startsolid)
			{
				overlapVector[j]->AddFuncNavCostEntity(cost);
			}
		}
	}
}


//--------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------
// Return pathfind cost multiplier for the given actor
float CFuncNavAvoid::GetCostMultiplier( edict_t *who ) const
{
	return IsApplicableTo(who) ? 25.0f : 1.0f;
}


//--------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------
// Return pathfind cost multiplier for the given actor
float CFuncNavPrefer::GetCostMultiplier( edict_t *who ) const
{
	return IsApplicableTo(who) ? 0.04f :	// 1/25th
			1.0f;
}


std::unordered_set<CFuncNavBlocker *> CFuncNavBlocker::gm_NavBlockers;

inline bool CFuncNavBlocker::IsBlockingNav( int teamNumber ) const
{
	if ( teamNumber == TEAM_ANY )
	{
		bool isBlocked = false;
		for ( int i=0; i<MAX_NAV_TEAMS; ++i )
		{
			isBlocked |= m_isBlockingNav[ i ];
		}

		return isBlocked;
	}
	return m_isBlockingNav[teamNumber % MAX_NAV_TEAMS];
}

//------------------------------------------------------------------------------
// Purpose : Add new entity positioned overlay text
// Input   : How many lines to offset text from origin
//			 The text to print
//			 How long to display text
//			 The color of the text
// Output  :
//------------------------------------------------------------------------------
void EntityText(ICollideable* collision, int text_offset, const char *text, float flDuration, int r = 255, int g = 255, int b = 255, int a = 255)
{
	if (collision == nullptr) {
		return;
	}
	Vector origin;
	Vector vecLocalCenter;
	Vector mins, maxs;
	collision->WorldSpaceSurroundingBounds(&mins, &maxs);
	VectorAdd(mins, maxs, vecLocalCenter);
	vecLocalCenter *= 0.5f;

	if ((collision->GetCollisionAngles() == vec3_angle) || (vecLocalCenter == vec3_origin))
	{
		VectorAdd(vecLocalCenter, collision->GetCollisionOrigin(), origin);
	}
	else
	{
		VectorTransform(vecLocalCenter, collision->CollisionToWorldTransform(), origin);
	}
	debugoverlay->AddTextOverlayRGB(origin, text_offset, NDEBUG_PERSIST_TILL_NEXT_SERVER, r, g, b, a, "%s", text);
}

int NavEntity::DrawDebugTextOverlays() {
	int offset = 1;
	char tempstr[512];
	extern IVEngineServer* engine;
	Q_snprintf(tempstr, sizeof(tempstr), "(%d) Name: %s", engine->IndexOfEdict(pEnt), pEnt->GetClassName());
	EntityText(pEnt->GetCollideable(), offset, tempstr, 0);
	offset++;
	auto serverEnt = pEnt->GetIServerEntity();
	if (serverEnt != nullptr) {
		auto modelName = serverEnt->GetModelName();
		if (modelName != NULL_STRING)
		{
			Q_snprintf(tempstr, sizeof(tempstr), "Model:%s", STRING(modelName));
			EntityText(pEnt->GetCollideable(), offset, tempstr, 0);
			offset++;
		}
	}
	Vector vecOrigin = pEnt->GetCollideable()->GetCollisionOrigin();
	Q_snprintf(tempstr, sizeof(tempstr), "Position: %0.1f, %0.1f, %0.1f\n", vecOrigin.x, vecOrigin.y, vecOrigin.z);
	EntityText(pEnt->GetCollideable(), offset, tempstr, 0);
	offset++;

	// TODO: Cross3D(EyePosition(), 16, 255, 0, 0, true, 0.05f);

	return offset;
}
//-----------------------------------------------------------------------------------------------------
int CFuncNavBlocker::DrawDebugTextOverlays( void )
{
	int offset = NavEntity::DrawDebugTextOverlays();
	// CFmtStr str;

	// FIRST_GAME_TEAM skips TEAM_SPECTATOR and TEAM_UNASSIGNED, so we can print
	// useful team names in a non-game-specific fashion.

	// TO-DO
	/* for (int i = FIRST_GAME_TEAM; i<FIRST_GAME_TEAM + MAX_NAV_TEAMS; ++i)
	{
		if ( IsBlockingNav( i ) )
		{		
			EntityText( pEnt->GetCollideable(), offset++, str.sprintf( "blocking team %d", i ), 0 );
		}
	} */

	NavAreaCollector collector( true );
	Extent extent;
	extent.Init( this->pEnt );
	TheNavMesh->ForAllAreasOverlappingExtent( collector, extent );

	for (auto area : collector.m_area)
	{
		Extent areaExtent;
		area->GetExtent(&areaExtent);
		debugoverlay->AddBoxOverlay(vec3_origin, areaExtent.lo, areaExtent.hi, vec3_angle, 0, 255, 0, 10, NDEBUG_PERSIST_TILL_NEXT_SERVER);
	}

	return offset;
}


//--------------------------------------------------------------------------------------------------------
void CFuncNavBlocker::UpdateBlocked()
{
	NavAreaCollector collector( true );
	Extent extent;
	extent.lo = m_CachedMins;
	extent.hi = m_CachedMaxs;
	TheNavMesh->ForAllAreasOverlappingExtent( collector, extent );

	for (auto area : collector.m_area)
	{
		area->UpdateBlocked(true);
	}
}


//--------------------------------------------------------------------------------------------------------
// Forces nav areas to unblock when the nav blocker is deleted (round restart) so flow can compute properly
void CFuncNavBlocker::UpdateOnRemove( void )
{
	UnblockNav();

	auto start = gm_NavBlockers.begin();
	auto end = gm_NavBlockers.end();
	auto pos = gm_NavBlockers.find(this);

	if (pos != end)
	{
		gm_NavBlockers.erase(this);
	}

	// gm_NavBlockers.FindAndRemove( this );

	// TODO: BaseClass::UpdateOnRemove();
}

//--------------------------------------------------------------------------------------------------------
CFuncNavBlocker::CFuncNavBlocker( edict_t* pEnt ) : NavEntity(pEnt),
		m_bDisabled(false), m_blockedTeamNumber(0)
{
	gm_NavBlockers.insert(this);

	if ( !m_blockedTeamNumber )
		m_blockedTeamNumber = TEAM_ANY;

	/**
	 * TODO
	SetMoveType( MOVETYPE_NONE );
	SetModel( STRING( GetModelName() ) );
	AddEffects( EF_NODRAW );
	SetCollisionGroup( COLLISION_GROUP_NONE );
	SetSolid( SOLID_NONE );
	AddSolidFlags( FSOLID_NOT_SOLID );
	*/
	pEnt->GetCollideable()->WorldSpaceSurroundingBounds( &m_CachedMins, &m_CachedMaxs );
	/*
	if ( m_bDisabled )
	{
		UnblockNav();
	}
	else
	{
		BlockNav();
	}
	*/
}

//--------------------------------------------------------------------------------------------------------
void CFuncNavBlocker::BlockNav( void )
{
	setBlockedTeam(true);
	Extent extent;
	extent.lo = m_CachedMins;
	extent.hi = m_CachedMaxs;
	TheNavMesh->ForAllAreasOverlappingExtent<CFuncNavBlocker>( *this, extent );
}

//--------------------------------------------------------------------------------------------------------
void CFuncNavBlocker::UnblockNav( void )
{
	setBlockedTeam(false);
	UpdateBlocked();
}

void CFuncNavBlocker::setBlockedTeam(bool block) {
	if ( m_blockedTeamNumber == TEAM_ANY )
	{
		for ( int i=0; i<MAX_NAV_TEAMS; ++i )
		{
			m_isBlockingNav[ i ] = block;
		}
	}
	else
	{
		m_isBlockingNav[ m_blockedTeamNumber % MAX_NAV_TEAMS ] = block;
	}
}

//--------------------------------------------------------------------------------------------------------
// functor that blocks areas in our extent
bool CFuncNavBlocker::operator()( CNavArea *area ) const
{
	area->MarkAsBlocked( m_blockedTeamNumber, pEnt, false );
	return true;
}


//--------------------------------------------------------------------------------------------------------
bool CFuncNavBlocker::CalculateBlocked( bool *pResultByTeam, const Vector &vecMins, const Vector &vecMaxs )
{
	int nTeamsBlocked = 0;
	int i;
	bool bBlocked = false;
	for ( i=0; i<MAX_NAV_TEAMS; ++i )
	{
		pResultByTeam[i] = false;
	}

	for (auto pBlocker : gm_NavBlockers)
	{
		bool bIsIntersecting = false;

		for (i = 0; i < MAX_NAV_TEAMS; ++i)
		{
			if (pBlocker->m_isBlockingNav[i] && !pResultByTeam[i]
				&& (bIsIntersecting
					|| !(bIsIntersecting = IsBoxIntersectingBox(
						pBlocker->m_CachedMins,
						pBlocker->m_CachedMaxs, vecMins, vecMaxs)))) {
				bBlocked = true;
				pResultByTeam[i] = true;
				nTeamsBlocked++;
			}
		}

		if (nTeamsBlocked == MAX_NAV_TEAMS)
		{
			break;
		}
	}

	return bBlocked;
}

//-----------------------------------------------------------------------------------------------------
int CFuncNavObstruction::DrawDebugTextOverlays( void )
{
	int offset = NavEntity::DrawDebugTextOverlays();
	EntityText(pEnt->GetCollideable(), offset++, CanObstructNavAreas() ? "Obstructing nav" : "Not obstructing nav",
		NDEBUG_PERSIST_TILL_NEXT_SERVER);
	return offset;
}

float CFuncNavObstruction::GetNavObstructionHeight(void) const {
	return JumpCrouchHeight;
}

//--------------------------------------------------------------------------------------------------------
void CFuncNavObstruction::UpdateOnRemove( void )
{
	TheNavMesh->UnregisterAvoidanceObstacle( this );

	//TODO: BaseClass::UpdateOnRemove();
}


//--------------------------------------------------------------------------------------------------------
CFuncNavObstruction::CFuncNavObstruction( edict_t* pEnt ) : NavEntity(pEnt)
{
	/*
	 * TODO
	SetMoveType( MOVETYPE_NONE );
	SetModel( STRING( GetModelName() ) );
	AddEffects( EF_NODRAW );
	SetCollisionGroup( COLLISION_GROUP_NONE );
	SetSolid( SOLID_NONE );
	AddSolidFlags( FSOLID_NOT_SOLID );
	 */
	if ( !m_bDisabled )
	{
		ObstructNavAreas();
		TheNavMesh->RegisterAvoidanceObstacle( this );
	}
}


//--------------------------------------------------------------------------------------------------------
void CFuncNavObstruction::InputEnable( )
{
	m_bDisabled = false;
	ObstructNavAreas();
	TheNavMesh->RegisterAvoidanceObstacle( this );
}


//--------------------------------------------------------------------------------------------------------
void CFuncNavObstruction::InputDisable( )
{
	m_bDisabled = true;
	TheNavMesh->UnregisterAvoidanceObstacle( this );
}


//--------------------------------------------------------------------------------------------------------
void CFuncNavObstruction::ObstructNavAreas( void )
{
	Extent extent;
	extent.Init( pEnt );
	TheNavMesh->ForAllAreasOverlappingExtent( *this, extent );
}


//--------------------------------------------------------------------------------------------------------
// functor that blocks areas in our extent
bool CFuncNavObstruction::operator()( CNavArea *area )
{
	area->MarkObstacleToAvoid( GetNavObstructionHeight() );
	return true;
}


//--------------------------------------------------------------------------------------------------------------
