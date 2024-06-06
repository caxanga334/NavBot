#include <algorithm>
#include <extension.h>
#include <util/helpers.h>
#include <server_class.h>
#include <sm_argbuffer.h>
#include <entities/baseentity.h>
#include <model_types.h>
#include "sdk_traces.h"

namespace trace
{
	inline static void ExtractHandleEntity(IHandleEntity* pHE, CBaseEntity** outEntity, edict_t** outEdict, int& entity)
	{
		IServerUnknown* unk = reinterpret_cast<IServerUnknown*>(pHE);
		IServerNetworkable* net = unk->GetNetworkable();

		if (staticpropmgr->IsStaticProp(pHE))
		{
			entity = INVALID_EHANDLE_INDEX;
			*outEntity = nullptr;
			*outEdict = nullptr;
			return;
		}

		if (net != nullptr)
		{
			*outEdict = net->GetEdict();
		}

		*outEntity = unk->GetBaseEntity();
		entity = gamehelpers->EntityToBCompatRef(unk->GetBaseEntity());
	}

	inline static bool StandardFilterRules(edict_t* pEntity, int contentsMask)
	{
		// For now we get the ICollideable from edicts
		if (pEntity == nullptr)
		{
			return false;
		}

		ICollideable* collide = pEntity->GetCollideable();

		if (collide == nullptr)
		{
			return false;
		}

		SolidType_t solid = collide->GetSolid();
		auto model = collide->GetCollisionModel();

		if ((modelinfo->GetModelType(model) != mod_brush) || (solid != SOLID_BSP && solid != SOLID_VPHYSICS))
		{
			if ((contentsMask & CONTENTS_MONSTER) == 0)
			{
				return false;
			}
		}

		entities::HBaseEntity baseentity(pEntity);

		// This code is used to cull out tests against see-thru entities
		if (!(contentsMask & CONTENTS_WINDOW) && baseentity.GetRenderMode() != kRenderNormal)
		{
			return false;
		}

		// FIXME: this is to skip BSP models that are entities that can be 
		// potentially moved/deleted, similar to a monster but doors don't seem to 
		// be flagged as monsters
		// FIXME: the FL_WORLDBRUSH looked promising, but it needs to be set on 
		// everything that's actually a worldbrush and it currently isn't
		if (!(contentsMask & CONTENTS_MOVEABLE) && (baseentity.GetMoveType() == MOVETYPE_PUSH))// !(touch->flags & FL_WORLDBRUSH) )
		{
			return false;
		}
		
		return true;
	}

	inline static bool PassServerEntityFilter(CBaseEntity* pTouch, CBaseEntity* pPass)
	{
		if (!pTouch || !pPass)
			return true;

		if (pTouch == pPass)
			return false;

		entities::HBaseEntity hTouch(pTouch);

		// don't clip against own missiles
		if (hTouch.GetOwnerEntity() == pPass)
		{
			return false;
		}

		entities::HBaseEntity hPass(pPass);
		
		// don't clip against owner
		if (hPass.GetOwnerEntity() == pTouch)
		{
			return false;
		}

		return true;
	}

	static bool CBaseEntity_ShouldCollide(CBaseEntity* entity, int collisiongroup, int contentsmask)
	{
		static int vtable_offset = -1;
		static SourceMod::ICallWrapper* pCall = nullptr;

		if (vtable_offset == -2) { return false; }

		if (vtable_offset == -1)
		{
			// initialize
			SourceMod::IGameConfig* cfg;
			char buffer[256];
			
			if (!gameconfs->LoadGameConfigFile("sdkhooks.games", &cfg, buffer, sizeof(buffer)))
			{
				smutils->LogError(myself, "CBaseEntity_ShouldCollide failed to load SDK Hooks gamedata! %s", buffer);
				vtable_offset = -2;
				return false;
			}

			if (!cfg->GetOffset("ShouldCollide", &vtable_offset))
			{
				smutils->LogError(myself, "Failed to get CBaseEntity::ShouldCollide offset from SDK Hooks gamedata!");
				vtable_offset = -2;
				gameconfs->CloseGameConfigFile(cfg);
				return false;
			}

			gameconfs->CloseGameConfigFile(cfg);
		}

		if (pCall == nullptr)
		{
			SourceMod::PassInfo ret;
			ret.flags = PASSFLAG_BYVAL;
			ret.size = sizeof(bool);
			ret.type = PassType_Basic;
			SourceMod::PassInfo params[2];
			params[0].flags = PASSFLAG_BYVAL;
			params[0].size = sizeof(int);
			params[0].type = PassType_Basic;
			params[1].flags = PASSFLAG_BYVAL;
			params[1].size = sizeof(int);
			params[1].type = PassType_Basic;

			pCall = g_pBinTools->CreateVCall(vtable_offset, 0, 0, &ret, params, 2);

#ifdef EXT_DEBUG
			smutils->LogMessage(myself, "Initialized gamedata for Virtual call CBaseEntity::ShouldCollide");
#endif // EXT_DEBUG
		}

		ArgBuffer<void*, int, int> vstk(entity, collisiongroup, contentsmask);
		bool retval = false;
		pCall->Execute(vstk, &retval);
		return retval;
	}

	// Local copy of the gamerules ShouldCollide
	// TO-DO: ShouldCollide is virtual, should be easy to SDKCall it instead of doing this
	inline static bool CGameRules_ShouldCollide(int collisionGroup0, int collisionGroup1)
	{
		if (collisionGroup0 > collisionGroup1)
		{
			// swap so that lowest is always first
			std::swap(collisionGroup0, collisionGroup1);
		}

#if SOURCE_ENGINE == SE_HL2DM
		if ((collisionGroup0 == COLLISION_GROUP_PLAYER || collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT) &&
			collisionGroup1 == COLLISION_GROUP_WEAPON)
		{
			return false;
		}

		if ((collisionGroup0 == COLLISION_GROUP_PLAYER || collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT) &&
			collisionGroup1 == COLLISION_GROUP_PUSHAWAY)
		{
			return false;
		}
#endif // SOURCE_ENGINE == SE_HL2DM

		if (collisionGroup0 == COLLISION_GROUP_DEBRIS && collisionGroup1 == COLLISION_GROUP_PUSHAWAY)
		{
			// let debris and multiplayer objects collide
			return true;
		}

		// --------------------------------------------------------------------------
		// NOTE: All of this code assumes the collision groups have been sorted!!!!
		// NOTE: Don't change their order without rewriting this code !!!
		// --------------------------------------------------------------------------

		// Don't bother if either is in a vehicle...
		if ((collisionGroup0 == COLLISION_GROUP_IN_VEHICLE) || (collisionGroup1 == COLLISION_GROUP_IN_VEHICLE))
			return false;

		if ((collisionGroup1 == COLLISION_GROUP_DOOR_BLOCKER) && (collisionGroup0 != COLLISION_GROUP_NPC))
			return false;

		if ((collisionGroup0 == COLLISION_GROUP_PLAYER) && (collisionGroup1 == COLLISION_GROUP_PASSABLE_DOOR))
			return false;

		if (collisionGroup0 == COLLISION_GROUP_DEBRIS || collisionGroup0 == COLLISION_GROUP_DEBRIS_TRIGGER)
		{
			// put exceptions here, right now this will only collide with COLLISION_GROUP_NONE
			return false;
		}

		// Dissolving guys only collide with COLLISION_GROUP_NONE
		if ((collisionGroup0 == COLLISION_GROUP_DISSOLVING) || (collisionGroup1 == COLLISION_GROUP_DISSOLVING))
		{
			if (collisionGroup0 != COLLISION_GROUP_NONE)
				return false;
		}

		// doesn't collide with other members of this group
		// or debris, but that's handled above
		if (collisionGroup0 == COLLISION_GROUP_INTERACTIVE_DEBRIS && collisionGroup1 == COLLISION_GROUP_INTERACTIVE_DEBRIS)
			return false;

#if SOURCE_ENGINE != SE_HL2DM
		// This change was breaking HL2DM
		// Adrian: TEST! Interactive Debris doesn't collide with the player.
		if (collisionGroup0 == COLLISION_GROUP_INTERACTIVE_DEBRIS && (collisionGroup1 == COLLISION_GROUP_PLAYER_MOVEMENT || collisionGroup1 == COLLISION_GROUP_PLAYER))
			return false;
#endif

		if (collisionGroup0 == COLLISION_GROUP_BREAKABLE_GLASS && collisionGroup1 == COLLISION_GROUP_BREAKABLE_GLASS)
			return false;

		// interactive objects collide with everything except debris & interactive debris
		if (collisionGroup1 == COLLISION_GROUP_INTERACTIVE && collisionGroup0 != COLLISION_GROUP_NONE)
			return false;

		// Projectiles hit everything but debris, weapons, + other projectiles
		if (collisionGroup1 == COLLISION_GROUP_PROJECTILE)
		{
			if (collisionGroup0 == COLLISION_GROUP_DEBRIS ||
				collisionGroup0 == COLLISION_GROUP_WEAPON ||
				collisionGroup0 == COLLISION_GROUP_PROJECTILE)
			{
				return false;
			}
		}

		// Don't let vehicles collide with weapons
		// Don't let players collide with weapons...
		// Don't let NPCs collide with weapons
		// Weapons are triggers, too, so they should still touch because of that
		if (collisionGroup1 == COLLISION_GROUP_WEAPON)
		{
			if (collisionGroup0 == COLLISION_GROUP_VEHICLE ||
				collisionGroup0 == COLLISION_GROUP_PLAYER ||
				collisionGroup0 == COLLISION_GROUP_NPC)
			{
				return false;
			}
		}

		// collision with vehicle clip entity??
		if (collisionGroup0 == COLLISION_GROUP_VEHICLE_CLIP || collisionGroup1 == COLLISION_GROUP_VEHICLE_CLIP)
		{
			// yes then if it's a vehicle, collide, otherwise no collision
			// vehicle sorts lower than vehicle clip, so must be in 0
			if (collisionGroup0 == COLLISION_GROUP_VEHICLE)
				return true;
			// vehicle clip against non-vehicle, no collision
			return false;
		}

		return true;
	}

	bool CBaseTraceFilter::ShouldHitEntity(IHandleEntity* pEntity, int contentsMask)
	{
		int entityindex = INVALID_EHANDLE_INDEX;
		CBaseEntity* baseentity = nullptr;
		edict_t* edict = nullptr;

		ExtractHandleEntity(pEntity, &baseentity, &edict, entityindex);

		return ShouldHitEntity(entityindex, baseentity, edict, contentsMask);
	}

	bool CTraceFilterPlayersOnly::ShouldHitEntity(int entity, CBaseEntity* pEntity, edict_t* pEdict, const int contentsMask)
	{
		if (CTraceFilterSimple::ShouldHitEntity(entity, pEntity, pEdict, contentsMask))
		{
			if (entity > 0 && entity <= gpGlobals->maxClients)
			{
				if (m_team >= 0 && pEdict != nullptr)
				{
					IPlayerInfo* info = playerinfomanager->GetPlayerInfo(pEdict);

					if (info != nullptr && info->GetTeamIndex() == m_team)
					{
						return false; // don't hit players from this team
					}
				}

				return true; // Hit players
			}
		}

		return false; // Don't hit anything else
	}

	bool CTraceFilterSimple::ShouldHitEntity(int entity, CBaseEntity* pEntity, edict_t* pEdict, const int contentsMask)
	{
		if (pEntity == nullptr)
		{
			return false;
		}

		if (pEdict != nullptr && !StandardFilterRules(pEdict, contentsMask))
			return false;

		if (m_passEntity != nullptr)
		{
			if (!PassServerEntityFilter(pEntity, m_passEntity))
			{
				return false;
			}
		}

		// Virtual call to CBaseEntity::ShouldCollide
		if (!CBaseEntity_ShouldCollide(pEntity, m_collisiongroup, contentsMask))
		{
			return false;
		}

		// TO-DO: Replace with a Virtual call to the real CGameRules::ShouldCollide
		if (pEdict != nullptr && pEdict->GetCollideable() != nullptr && !CGameRules_ShouldCollide(m_collisiongroup, pEdict->GetCollideable()->GetCollisionGroup()))
		{
			return false;
		}

		if (m_extraHitFunc)
		{
			if (!m_extraHitFunc(entity, pEntity, pEdict, contentsMask))
			{
				return false;
			}
		}

		return true;
	}

	inline static bool IsNPC(CBaseEntity* npc)
	{
		// New gamehelpers function, see: https://github.com/alliedmodders/sourcemod/commit/7df2f8e0457ce00426f8fa414166db41939c3efd
		// This will make navbot require a very recent build of SM 1.12
		ServerClass* pServerClass = UtilHelpers::FindEntityServerClass(npc);

		if (pServerClass == nullptr)
		{
			return false;
		}

		if (UtilHelpers::HasDataTable(pServerClass->m_pTable, "DT_AI_BaseNPC"))
		{
			return true;
		}

#if SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_CSGO
		// CS Hostages
		if (UtilHelpers::HasDataTable(pServerClass->m_pTable, "DT_CHostage"))
		{
			return true;
		}
#endif // SOURCE_ENGINE == SE_CSS

#if SOURCE_ENGINE == SE_TF2 || SOURCE_ENGINE == SE_LEFT4DEAD || SOURCE_ENGINE == SE_LEFT4DEAD2
		// Also ignore NextBot NPCs
		if (UtilHelpers::HasDataTable(pServerClass->m_pTable, "DT_NextBot"))
		{
			return true;
		}
#endif // SOURCE_ENGINE == SE_TF2 || SOURCE_ENGINE == SE_LEFT4DEAD || SOURCE_ENGINE == SE_LEFT4DEAD2
		
		return false;
	}

	bool CTraceFilterNoNPCsOrPlayers::ShouldHitEntity(int entity, CBaseEntity* pEntity, edict_t* pEdict, const int contentsMask)
	{
		if (CTraceFilterSimple::ShouldHitEntity(entity, pEntity, pEdict, contentsMask))
		{
			if (entity > 0 && entity <= gpGlobals->maxClients)
			{
				return false;
			}

			if (IsNPC(pEntity))
			{
				return false;
			}
		}

		return true;
	}

	inline static bool IsEntityWalkable(CBaseEntity* pEntity, unsigned int flags)
	{
		entities::HBaseEntity be(pEntity);

		if (UtilHelpers::FClassnameIs(pEntity, "worldspawn"))
			return false;

		if (UtilHelpers::FClassnameIs(pEntity, "player"))
			return false;

		// if we hit a door, assume its walkable because it will open when we touch it
		if (UtilHelpers::FClassnameIs(pEntity, "func_door*"))
		{
#ifdef PROBLEMATIC	// cp_dustbowl doors dont open by touch - they use surrounding triggers
			if (!entity->HasSpawnFlags(SF_DOOR_PTOUCH))
			{
				// this door is not opened by touching it, if it is closed, the area is blocked
				CBaseDoor* door = (CBaseDoor*)entity;
				return door->m_toggle_state == TS_AT_TOP;
			}
#endif // _DEBUG

			return (flags & WALK_THRU_FUNC_DOORS) ? true : false;
		}

		if (UtilHelpers::FClassnameIs(pEntity, "prop_door*"))
		{
			return (flags & WALK_THRU_PROP_DOORS) ? true : false;
		}

		// if we hit a clip brush, ignore it if it is not BRUSHSOLID_ALWAYS
		if (UtilHelpers::FClassnameIs(pEntity, "func_brush"))
		{
			entities::HFuncBrush brush(pEntity);

			switch (brush.GetSolidity())
			{
			case entities::HFuncBrush::BrushSolidities_e::BRUSHSOLID_ALWAYS:
				return false;
			case entities::HFuncBrush::BrushSolidities_e::BRUSHSOLID_NEVER:
				return true;
			case entities::HFuncBrush::BrushSolidities_e::BRUSHSOLID_TOGGLE:
			default:
				return (flags & WALK_THRU_TOGGLE_BRUSHES) ? true : false;
			}
		}

		// if we hit a breakable object, assume its walkable because we will shoot it when we touch it
		if (UtilHelpers::FClassnameIs(pEntity, "func_breakable") && be.GetHealth() > 0 && be.GetTakeDamage() == DAMAGE_YES)
			return (flags & WALK_THRU_BREAKABLES) ? true : false;

		if (UtilHelpers::FClassnameIs(pEntity, "func_breakable_surf") && be.GetTakeDamage() == DAMAGE_YES)
			return (flags & WALK_THRU_BREAKABLES) ? true : false;

		if (UtilHelpers::FClassnameIs(pEntity, "func_playerinfected_clip") == true)
			return true;

		ConVarRef solidprops("sm_nav_solid_props", false);

		if (solidprops.GetBool() && UtilHelpers::FClassnameIs(pEntity, "prop_*"))
			return true;

		return false;
	}

	bool CTraceFilterWalkableEntities::ShouldHitEntity(int entity, CBaseEntity* pEntity, edict_t* pEdict, const int contentsMask)
	{
		if (CTraceFilterNoNPCsOrPlayers::ShouldHitEntity(entity, pEntity, pEdict, contentsMask))
		{
			return !IsEntityWalkable(pEntity, m_flags);
		}

		return false;
	}
}

