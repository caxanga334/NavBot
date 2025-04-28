#include <algorithm>
#include <extension.h>
#include <util/helpers.h>
#include <util/sdkcalls.h>
#include <util/entprops.h>
#include <ISDKTools.h>
#include <server_class.h>
#include <sm_argbuffer.h>
#include <entities/baseentity.h>
#include <model_types.h>
#include "sdk_traces.h"

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

namespace trace
{
	const CBaseEntity* EntityFromEntityHandle(const IHandleEntity* pConstHandleEntity)
	{
		IHandleEntity* pHandleEntity = const_cast<IHandleEntity*>(pConstHandleEntity);

		if (staticpropmgr->IsStaticProp(pHandleEntity))
		{
			return nullptr;
		}

		IServerUnknown* pUnk = reinterpret_cast<IServerUnknown*>(pHandleEntity);
		return pUnk->GetBaseEntity();
	}

	CBaseEntity* EntityFromEntityHandle(IHandleEntity* pHandleEntity)
	{
		if (staticpropmgr->IsStaticProp(pHandleEntity))
		{
			return nullptr;
		}

		IServerUnknown* pUnk = reinterpret_cast<IServerUnknown*>(pHandleEntity);
		return pUnk->GetBaseEntity();
	}

	void ExtractHandleEntity(IHandleEntity* pHandleEntity, CBaseEntity** outEntity, edict_t** outEdict, int& entity)
	{
		CBaseEntity* pEntity = EntityFromEntityHandle(pHandleEntity);

		if (!pEntity) // static prop
		{
			entity = INVALID_EHANDLE_INDEX;
			*outEntity = nullptr;
			*outEdict = nullptr;
			return;
		}

		*outEntity = pEntity;
		*outEdict = servergameents->BaseEntityToEdict(pEntity);
		entity = gamehelpers->EntityToBCompatRef(pEntity);
	}

	static bool StandardFilterRules(IHandleEntity* pHandleEntity, int contentsMask)
	{
#ifdef EXT_VPROF_ENABLED
		VPROF_BUDGET("trace::StandardFilterRules", "NavBot");
#endif // EXT_VPROF_ENABLED

		CBaseEntity* pCollide = EntityFromEntityHandle(pHandleEntity);

		// Static prop case
		if (!pCollide)
		{
			return true;
		}

		ICollideable* collider = reinterpret_cast<IServerEntity*>(pCollide)->GetCollideable();

		SolidType_t solid = collider->GetSolid();
		const model_t* pModel = collider->GetCollisionModel();

		if (modelinfo->GetModelType(pModel) != static_cast<int>(modtype_t::mod_brush) || (solid != SolidType_t::SOLID_BSP && solid != SolidType_t::SOLID_VPHYSICS))
		{
			if ((contentsMask & CONTENTS_MONSTER) == 0)
			{
				return false;
			}
		}

		entities::HBaseEntity baseentity(pCollide);

		// This code is used to cull out tests against see-thru entities
		if (!(contentsMask & CONTENTS_WINDOW) && baseentity.IsTransparent())
			return false;

		// FIXME: this is to skip BSP models that are entities that can be 
		// potentially moved/deleted, similar to a monster but doors don't seem to 
		// be flagged as monsters
		// FIXME: the FL_WORLDBRUSH looked promising, but it needs to be set on 
		// everything that's actually a worldbrush and it currently isn't
		if (!(contentsMask & CONTENTS_MOVEABLE) && (baseentity.GetMoveType() == MoveType_t::MOVETYPE_PUSH))// !(touch->flags & FL_WORLDBRUSH) )
			return false;
		
		return true;
	}

	inline static bool PassServerEntityFilter(IHandleEntity* pTouch, IHandleEntity* pPass)
	{
#ifdef EXT_VPROF_ENABLED
		VPROF_BUDGET("trace::PassServerEntityFilter", "NavBot");
#endif // EXT_VPROF_ENABLED

		if (!pPass)
			return true;

		if (pTouch == pPass)
			return false;

		CBaseEntity* pEntTouch = EntityFromEntityHandle(pTouch);
		CBaseEntity* pEntPass = EntityFromEntityHandle(pPass);
		if (!pEntTouch || !pEntPass)
			return true;

		entities::HBaseEntity bTouch(pEntTouch);
		entities::HBaseEntity bPass(pEntPass);

		// don't clip against own missiles
		if (bTouch.GetOwnerEntity() == pEntPass)
			return false;

		// don't clip against owner
		if (bPass.GetOwnerEntity() == pEntTouch)
			return false;


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
	// This is used as a fallback in case the SDKCall is not available.
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

	bool CTraceFilterSimple::ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask)
	{
#ifdef EXT_VPROF_ENABLED
		VPROF_BUDGET("trace::CTraceFilterSimple::ShouldHitEntity", "NavBot");
#endif // EXT_VPROF_ENABLED

		if (!StandardFilterRules(pHandleEntity, contentsMask))
		{
			return false;
		}

		if (m_passEntity)
		{
			IHandleEntity* pPass = reinterpret_cast<IHandleEntity*>(m_passEntity);

			if (!PassServerEntityFilter(pHandleEntity, pPass))
			{
				return false;
			}
		}

		CBaseEntity* pEntity = EntityFromEntityHandle(pHandleEntity);

		if (!pEntity)
		{
			return false;
		}

		if (!CBaseEntity_ShouldCollide(pEntity, m_collisiongroup, contentsMask))
		{
			return false;
		}

		CGameRules* pGameRules = reinterpret_cast<CGameRules*>(g_pSDKTools->GetGameRules());

		if (pGameRules)
		{
			ICollideable* collider = reinterpret_cast<IServerEntity*>(pEntity)->GetCollideable();

			if (!sdkcalls->CGameRules_ShouldCollide(pGameRules, m_collisiongroup, collider->GetCollisionGroup()))
			{
				return false;
			}
		}
		else
		{
			ICollideable* collider = reinterpret_cast<IServerEntity*>(pEntity)->GetCollideable();

			if (!CGameRules_ShouldCollide(m_collisiongroup, collider->GetCollisionGroup()))
			{
				return false;
			}
		}

		if (m_extraHitFunc)
		{
			if (!m_extraHitFunc(pHandleEntity, contentsMask))
			{
				return false;
			}
		}

		return true;
	}

	bool CTraceFilterOnlyNPCsAndPlayer::ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask)
	{
#ifdef EXT_VPROF_ENABLED
		VPROF_BUDGET("trace::CTraceFilterOnlyNPCsAndPlayer::ShouldHitEntity", "NavBot");
#endif // EXT_VPROF_ENABLED

		if (CTraceFilterSimple::ShouldHitEntity(pHandleEntity, contentsMask))
		{
			CBaseEntity* pEntity = EntityFromEntityHandle(pHandleEntity);

			if (!pEntity)
			{
				return false;
			}

#if SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_CSGO
			// CSS/CSGO: Hostages doesn't derive from CAI_BaseNPC class
			if (UtilHelpers::FClassnameIs(pEntity, "hostage_entity"))
			{
				return true;
			}
#endif // SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_CSGO

			int index = gamehelpers->EntityToBCompatRef(pEntity);

			// is player
			if (index > 0 && index <= gpGlobals->maxClients)
			{
				return true;
			}

			datamap_t* pDataMap = gamehelpers->GetDataMap(pEntity);
			SourceMod::sm_datatable_info_t info;

			if (pDataMap && gamehelpers->FindDataMapInfo(pDataMap, "m_NPCState", &info))
			{
				// if m_NPCState exists, this entity derives from CAI_BaseNPC
				return true;
			}

			return false;
		}

		return false;
	}

	bool CTraceFilterPlayersOnly::ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask)
	{
#ifdef EXT_VPROF_ENABLED
		VPROF_BUDGET("trace::CTraceFilterPlayersOnly::ShouldHitEntity", "NavBot");
#endif // EXT_VPROF_ENABLED

		if (CTraceFilterSimple::ShouldHitEntity(pHandleEntity, contentsMask))
		{
			CBaseEntity* pEntity = EntityFromEntityHandle(pHandleEntity);

			if (!pEntity)
			{
				return false;
			}

			int index = gamehelpers->EntityToBCompatRef(pEntity);

			// is player
			if (index > 0 && index <= gpGlobals->maxClients)
			{
				if (m_team >= 0)
				{
					if (entityprops::GetEntityTeamNum(pEntity) == m_team)
					{
						// don't hit players from the passed team index
						return false;
					}
				}

				return true;
			}

			return false;
		}

		return false;
	}

	bool CTraceFilterNoNPCsOrPlayers::ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask)
	{
#ifdef EXT_VPROF_ENABLED
		VPROF_BUDGET("trace::CTraceFilterNoNPCsOrPlayers::ShouldHitEntity", "NavBot");
#endif // EXT_VPROF_ENABLED

		if (CTraceFilterSimple::ShouldHitEntity(pHandleEntity, contentsMask))
		{
			CBaseEntity* pEntity = EntityFromEntityHandle(pHandleEntity);

			if (!pEntity)
			{
				return false;
			}

#if SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_CSGO
			if (UtilHelpers::FClassnameIs(pEntity, "hostage_entity"))
			{
				return false;
			}
#endif // SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_CSGO

			int index = gamehelpers->EntityToBCompatRef(pEntity);

			// is player
			if (index > 0 && index <= gpGlobals->maxClients)
			{
				return false;
			}

			datamap_t* pDataMap = gamehelpers->GetDataMap(pEntity);
			SourceMod::sm_datatable_info_t info;

			if (pDataMap && gamehelpers->FindDataMapInfo(pDataMap, "m_NPCState", &info))
			{
				// if m_NPCState exists, this entity derives from CAI_BaseNPC
				return false;
			}

			return true;
		}

		return false;
	}
}

