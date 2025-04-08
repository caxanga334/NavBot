#include "extension.h"
#include <PlayerState.h>
#include "navmesh/nav_mesh.h"
#include "navmesh/nav_area.h"
#include <entities/baseentity.h>
#include <util/entprops.h>
#include <util/helpers.h>
#include <util/sdkcalls.h>
#include "extplayer.h"

#ifdef EXT_DEBUG
#include <sdkports/debugoverlay_shared.h>
#endif // EXT_DEBUG

constexpr auto PLAYER_NAV_UPDATE_TIME = 0.1f; // Interval of nav mesh data updates
constexpr auto NEAREST_AREA_MAX_DISTANCE = 128.0f;

CBaseExtPlayer::CBaseExtPlayer(edict_t* edict)
{
	m_edict = edict;
	m_pEntity = edict->GetIServerEntity()->GetBaseEntity();
	m_index = gamehelpers->IndexOfEdict(edict);
	m_playerinfo = playerinfomanager->GetPlayerInfo(edict);
	m_lastnavarea = nullptr;
	m_navupdatetimer = 6;

#ifdef EXT_DEBUG
	if (!UtilHelpers::IsPlayerIndex(m_index))
	{
		// this class only supports players, on debug mode, log this if it happens
		// place a breakpoint here to catch in debugger
		smutils->LogError(myself, "CBaseExtPlayer constructed on non-player entity!");
	}
#endif // EXT_DEBUG
}

CBaseExtPlayer::~CBaseExtPlayer()
{
}

bool CBaseExtPlayer::operator==(const CBaseExtPlayer& other) const
{
	// Maybe also add something like a userid here?
	return this->GetIndex() == other.GetIndex();
}

void CBaseExtPlayer::PlayerThink()
{
	UpdateLastKnownNavArea();
}

void CBaseExtPlayer::UpdateLastKnownNavArea(const bool forceupdate)
{
	// https://cs.alliedmods.net/hl2sdk-csgo/source/game/server/basecombatcharacter.cpp#3351

	if (forceupdate)
	{
		m_navupdatetimer = -1;
	}

	if (m_navupdatetimer > 0)
	{
		m_navupdatetimer--;
		return;
	}
	
	CBaseEntity* groundent = GetGroundEntity();

	if (groundent == nullptr)
	{
		return; // don't update if the bot is midair
	}

	CNavArea* newarea = TheNavMesh->GetNearestNavArea(GetEdict(), GETNAVAREA_CHECK_GROUND | GETNAVAREA_CHECK_LOS, 50.0f);

	if (!newarea)
	{
		return;
	}

	if (newarea != m_lastnavarea)
	{
		NavAreaChanged(m_lastnavarea, newarea);
		m_lastnavarea = newarea;
	}

	m_navupdatetimer = TIME_TO_TICKS(PLAYER_NAV_UPDATE_TIME);
}

const Vector CBaseExtPlayer::WorldSpaceCenter() const
{
	return UtilHelpers::getWorldSpaceCenter(GetEdict());
}

const Vector CBaseExtPlayer::GetAbsOrigin() const
{
	return m_playerinfo->GetAbsOrigin();
}

const QAngle CBaseExtPlayer::GetAbsAngles() const
{
	return m_playerinfo->GetAbsAngles();
}

const Vector CBaseExtPlayer::GetEyeOrigin() const
{
	Vector ear;
	gameclients->ClientEarPosition(m_edict, &ear);
	return ear;
}

const QAngle CBaseExtPlayer::GetEyeAngles() const
{
	// TO-DO: This is the 'RCBot' way of getting eye angles
	// Sourcemod VCalls EyeAngles.
	// Determine which is better

	auto cmd = m_playerinfo->GetLastUserCommand();
	return cmd.viewangles;
}

const Vector CBaseExtPlayer::GetMins() const
{
	Vector result;

	if (entprops->GetEntPropVector(GetIndex(), Prop_Send, "m_vecMins", result) == true)
	{
		return result;
	}

	return vec3_origin;
}

const Vector CBaseExtPlayer::GetMaxs() const
{
	Vector result;

	if (entprops->GetEntPropVector(GetIndex(), Prop_Send, "m_vecMaxs", result) == true)
	{
		return result;
	}

	return vec3_origin;
}

void CBaseExtPlayer::GetHeadShotPosition(const char* bonename, Vector& result) const
{
	std::unique_ptr<CStudioHdr> studiohdr = UtilHelpers::GetEntityModelPtr(GetEdict());

	if (studiohdr)
	{
		int bone = UtilHelpers::LookupBone(studiohdr.get(), bonename);

		if (bone >= 0)
		{
			Vector position;
			QAngle angle;

			if (UtilHelpers::GetBonePosition(GetEntity(), bone, position, angle))
			{
				result = position; // aim at the head bone
				return;
			}
		}
	}

	// bone aim failed
	result = GetEyeOrigin();
	result.z -= 0.5f;
	return;
}

void CBaseExtPlayer::EyeVectors(Vector* pForward) const
{
	auto eyeangles = GetEyeAngles();
	AngleVectors(eyeangles, pForward);
}

void CBaseExtPlayer::EyeVectors(Vector* pForward, Vector* pRight, Vector* pUp) const
{
	auto eyeangles = GetEyeAngles();
	AngleVectors(eyeangles, pForward, pRight, pUp);
}

const Vector CBaseExtPlayer::GetAbsVelocity() const
{
	Vector result;

	if (entprops->GetEntPropVector(GetIndex(), Prop_Data, "m_vecAbsVelocity", result) == false)
	{
		return vec3_origin;
	}

	return result;
}

void CBaseExtPlayer::SetAbsVelocity(const Vector& velocity) const
{
	entprops->SetEntPropVector(GetIndex(), Prop_Data, "m_vecAbsVelocity", velocity);
}

Vector CBaseExtPlayer::BodyDirection3D() const
{
	auto angles = BodyAngles();
	Vector bodydir;
	AngleVectors(angles, &bodydir);
	return bodydir;
}

Vector CBaseExtPlayer::BodyDirection2D() const
{
	auto body2d = BodyDirection3D();
	body2d.z = 0.0f;
	body2d.AsVector2D().NormalizeInPlace();
	return body2d;
}

float CBaseExtPlayer::GetModelScale() const
{
	float result = 1.0f;
	entprops->GetEntPropFloat(GetIndex(), Prop_Send, "m_flModelScale", result);
	return result; // result won't be touched if getentpropfloat fails, so this will default to 1.0
}

void CBaseExtPlayer::Teleport(const Vector& origin, QAngle* angles, Vector* velocity) const
{
	entities::HBaseEntity entity(GetEntity());
	entity.Teleport(origin, angles, velocity);
}

void CBaseExtPlayer::ChangeTeam(int newTeam)
{
	m_playerinfo->ChangeTeam(newTeam);
}

int CBaseExtPlayer::GetCurrentTeamIndex() const
{
	return m_playerinfo->GetTeamIndex();
}

MoveType_t CBaseExtPlayer::GetMoveType() const
{
	int movetype = 0;
	
	if (entprops->GetEntProp(GetIndex(), Prop_Send, "movetype", movetype) == false)
	{
		return MOVETYPE_WALK; // if lookup fails, default to walk
	}

	return static_cast<MoveType_t>(movetype);
}

CBaseEntity* CBaseExtPlayer::GetGroundEntity() const
{
	int groundent = -1;
	entprops->GetEntPropEnt(GetIndex(), Prop_Send, "m_hGroundEntity", groundent);
	return gamehelpers->ReferenceToEntity(groundent);
}

/**
 * @brief Given a weapon, return true if the player owns this weapon
 * @param weapon Weapon classname
 * @param result If found, the weapon edict will be stored here
 * @return true if the player owns this weapon
*/
bool CBaseExtPlayer::Weapon_OwnsThisType(const char* weapon, edict_t** result)
{
	int weapon_entity = -1;
	edict_t* entity = nullptr;
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		if (entprops->GetEntPropEnt(GetIndex(), Prop_Send, "m_hMyWeapons", weapon_entity, i) == false)
		{
			return false; // lookup failed
		}

		entity = gamehelpers->EdictOfIndex(weapon_entity);

		if (entity == nullptr)
			continue;

		const char* szclassname = gamehelpers->GetEntityClassname(entity);

		if (strcmp(weapon, szclassname) == 0)
		{
			if (result)
			{
				*result = entity;
			}

			return true;
		}
	}

	return false;
}

CBaseEntity* CBaseExtPlayer::GetActiveWeapon() const
{
	return entprops->GetCachedDataPtr<CHandle<CBaseEntity>>(GetEntity(), CEntPropUtils::CacheIndex::CBASECOMBATCHARACTER_ACTIVEWEAPON)->Get();
}

bool CBaseExtPlayer::IsActiveWeapon(const char* classname) const
{
	auto weapon = GetActiveWeapon();
	
	if (strcmp(gamehelpers->GetEntityClassname(weapon), classname) == 0)
	{
		return true;
	}

	return false;
}

int CBaseExtPlayer::GetAmmoOfIndex(int index) const
{
	int ammo = 0;
	entprops->GetEntProp(GetIndex(), Prop_Send, "m_iAmmo", ammo, 4, index);
	return ammo;
}

std::vector<edict_t*> CBaseExtPlayer::GetAllWeapons() const
{
	std::vector<edict_t*> myweapons;
	myweapons.reserve(MAX_WEAPONS);

	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		int weapon = INVALID_EHANDLE_INDEX;
		
		if (!entprops->GetEntPropEnt(GetIndex(), Prop_Send, "m_hMyWeapons", weapon, i))
			continue;

		auto entity = gamehelpers->EdictOfIndex(weapon);

		if (!entity)
			continue;

		myweapons.push_back(entity);
	}

	return myweapons;
}

float CBaseExtPlayer::GetMaxSpeed() const
{
	float speed = 0.0f;
	entprops->GetEntPropFloat(GetIndex(), Prop_Send, "m_flMaxspeed", speed);
	return speed;
}

bool CBaseExtPlayer::SelectWeapon(CBaseEntity* weapon) const
{
	return sdkcalls->CBaseCombatCharacter_Weapon_Switch(GetEntity(), weapon);
}

CBaseEntity* CBaseExtPlayer::GetWeaponOfSlot(int slot) const
{
	return sdkcalls->CBaseCombatCharacter_Weapon_GetSlot(GetEntity(), slot);
}

void CBaseExtPlayer::SnapEyeAngles(const QAngle& viewAngles) const
{
	CPlayerState* state = gameclients->GetPlayerState(GetEdict());

	if (state != nullptr)
	{
		state->v_angle = viewAngles;
		state->fixangle = FIXANGLE_ABSOLUTE;
	}
#ifdef EXT_DEBUG
	else
	{
		smutils->LogError(myself, "Got NULL CPlayerState!");
	}
#endif // EXT_DEBUG
}

bool CBaseExtPlayer::IsLookingTowards(edict_t* edict, const float tolerance) const
{
	entities::HBaseEntity be(edict);
	return IsLookingTowards(be.GetAbsOrigin(), tolerance) || IsLookingTowards(be.WorldSpaceCenter(), tolerance) || IsLookingTowards(be.EyePosition(), tolerance);
}

bool CBaseExtPlayer::IsLookingTowards(CBaseEntity* entity, const float tolerance) const
{
	entities::HBaseEntity be(entity);
	return IsLookingTowards(be.GetAbsOrigin(), tolerance) || IsLookingTowards(be.WorldSpaceCenter(), tolerance) || IsLookingTowards(be.EyePosition(), tolerance);
}

bool CBaseExtPlayer::IsLookingTowards(const Vector& position, const float tolerance) const
{
	Vector eyepos = GetEyeOrigin();
	Vector to = position - eyepos;
	to.NormalizeInPlace();
	Vector forward;
	EyeVectors(&forward);
	return DotProduct(forward, to) >= tolerance;
}


#ifdef EXT_DEBUG
CON_COMMAND_F(sm_navbot_debug_boners, "Debugs the CBaseAnimating::LookupBone port of the extension.", FCVAR_CHEAT)
{
	auto player = gamehelpers->EdictOfIndex(1); // Get listen server host

	if (!player)
	{
		rootconsole->ConsolePrint("Listen Server host is NULL!");
		return;
	}

	std::unique_ptr<char[]> bonename = std::make_unique<char[]>(256);

	if (args.ArgC() >= 2)
	{
		ke::SafeStrcpy(bonename.get(), 256U, args[1]);
	}
	else
	{
		ke::SafeStrcpy(bonename.get(), 256U, "ValveBiped.Bip01_Head1");
	}

	auto model = UtilHelpers::GetEntityModelPtr(player);

	if (!model)
	{
		rootconsole->ConsolePrint("Failed to get a model pointer!");
		return;
	}

	int bone = UtilHelpers::LookupBone(model.get(), bonename.get());

	rootconsole->ConsolePrint("Bone \"%s\" Lookup result = %i", bonename.get(), bone);

	if (bone < 0)
	{
		return;
	}

	Vector mins{ -1.0f, -1.0f, -1.0f };
	Vector maxs{ 1.0f, 1.0f, 1.0f };

	Vector pos;
	QAngle angles;
	
	if (!UtilHelpers::GetBonePosition(player->GetIServerEntity()->GetBaseEntity(), bone, pos, angles))
	{
		rootconsole->ConsolePrint("Warning: GetBonePosition is not available for the current mod!");
		return;
	}

	NDebugOverlay::Box(pos, mins, maxs, 255, 0, 0, 255, 10.0f);
}

CON_COMMAND(sm_navbot_debug_entprops, "Tests the ent prop lib.")
{
	constexpr int LISTEN_SERVER_HOST_ENTITY = 1;

	Vector vec_var;
	int integer_var = 0;
	int entity_var = 0;
	float float_var = 0.0f;
	char string_var[256]{};
	size_t length = 0;
	bool results[5]{};

	// Missing string prop, players doesn't have a good one to test

	results[0] = entprops->GetEntProp(LISTEN_SERVER_HOST_ENTITY, Prop_Send, "m_iTeamNum", integer_var);
	results[1] = entprops->GetEntPropFloat(LISTEN_SERVER_HOST_ENTITY, Prop_Send, "m_flModelScale", float_var);
	results[2] = entprops->GetEntPropEnt(LISTEN_SERVER_HOST_ENTITY, Prop_Send, "m_hActiveWeapon", entity_var);
	results[3] = entprops->GetEntPropVector(LISTEN_SERVER_HOST_ENTITY, Prop_Send, "m_vecOrigin", vec_var);
	results[4] = entprops->GetEntPropString(LISTEN_SERVER_HOST_ENTITY, Prop_Data, "m_iClassname", string_var, sizeof(string_var), length);

	for (int i = 0; i < 5; i++)
	{
		rootconsole->ConsolePrint("Result #%i was %s.", i, results[i] ? "success" : "failure");
	}

	rootconsole->ConsolePrint("Integer: %i Float: %f Vector: <%3.4f, %3.4f, %3.4f>", integer_var, float_var, vec_var.x, vec_var.y, vec_var.z);

	auto edict = gamehelpers->EdictOfIndex(entity_var);

	if (edict == nullptr)
	{
		rootconsole->ConsolePrint("Entity Var is %i but failed to get an edict pointer!", entity_var);
		return;
	}
	
	rootconsole->ConsolePrint("Entity var: #%i %p", entity_var, edict);

	rootconsole->ConsolePrint("String Var: '%s' Length: %i", string_var, length);
}

CON_COMMAND(sm_navbot_debug_player_positions, "Shows the player's Eye Origin, World Space Center and Absolute Origin")
{
	edict_t* player = UtilHelpers::GetListenServerHost();

	Vector eyes;
	gameclients->ClientEarPosition(player, &eyes);
	Vector center = UtilHelpers::getWorldSpaceCenter(player);
	const Vector& origin = player->GetCollideable()->GetCollisionOrigin();

	Vector mins{ -1.0f, -1.0f, -1.0f };
	Vector maxs{ 1.0f, 1.0f, 1.0f };

	NDebugOverlay::Box(eyes, mins, maxs, 255, 0, 0, 255, 10.0f);
	NDebugOverlay::Box(center, mins, maxs, 0, 255, 0, 255, 10.0f);
	NDebugOverlay::Box(origin, mins, maxs, 0, 0, 255, 255, 10.0f);
}

#endif // EXT_DEBUG
