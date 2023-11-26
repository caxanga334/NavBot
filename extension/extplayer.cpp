#include "extension.h"
#include "navmesh/nav_mesh.h"
#include <util/entprops.h>
#include <util/helpers.h>
#include "extplayer.h"

extern CGlobalVars* gpGlobals;
extern IPlayerInfoManager* playerinfomanager;
extern IServerGameClients* gameclients;
extern CNavMesh* TheNavMesh;

constexpr auto PLAYER_NAV_UPDATE_TIME = 0.6f; // Interval of nav mesh data updates
constexpr auto NEAREST_AREA_MAX_DISTANCE = 128.0f;

CBaseExtPlayer::CBaseExtPlayer(edict_t* edict)
{
	m_edict = edict;
	m_index = gamehelpers->IndexOfEdict(edict);
	m_playerinfo = playerinfomanager->GetPlayerInfo(edict);
	m_lastnavarea = nullptr;
	m_navupdatetimer = 64;
}

CBaseExtPlayer::~CBaseExtPlayer()
{
}

bool CBaseExtPlayer::operator==(const CBaseExtPlayer& other)
{
	// Maybe also add something like a userid here?
	return this->GetIndex() == other.GetIndex();
}

void CBaseExtPlayer::PlayerThink()
{
	if (--m_navupdatetimer <= 0)
	{
		m_navupdatetimer = TIME_TO_TICKS(PLAYER_NAV_UPDATE_TIME);
		auto origin = GetAbsOrigin();
		auto newarea = TheNavMesh->GetNearestNavArea(origin, NEAREST_AREA_MAX_DISTANCE, false, true, m_playerinfo->GetTeamIndex());

		if (newarea != m_lastnavarea)
		{
			NavAreaChanged(m_lastnavarea, newarea);
			m_lastnavarea = newarea;
		}
	}
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

void CBaseExtPlayer::ChangeTeam(int newTeam)
{
	m_playerinfo->ChangeTeam(newTeam);
}

int CBaseExtPlayer::GetCurrentTeamIndex() const
{
	return m_playerinfo->GetTeamIndex();
}

#ifdef SMNAV_DEBUG
CON_COMMAND_F(smnav_debug_boners, "Debugs the CBaseAnimating::LookupBone port of the extension.", FCVAR_CHEAT)
{
	auto player = gamehelpers->EdictOfIndex(1); // Get listen server host

	if (!player)
	{
		rootconsole->ConsolePrint("Listen Server host is NULL!");
		return;
	}

	auto model = UtilHelpers::GetEntityModelPtr(player);

	if (!model)
	{
		rootconsole->ConsolePrint("Failed to get a model pointer!");
		return;
	}

	int bone = UtilHelpers::LookupBone(model, "ValveBiped.Bip01_Head1");

	rootconsole->ConsolePrint("Bone Lookup result = %i", bone);
	delete model;
}

CON_COMMAND(smnav_debug_entprops, "Tests the ent prop lib.")
{
	constexpr int LISTEN_SERVER_HOST_ENTITY = 1;

	Vector vec_var;
	int integer_var = 0;
	int entity_var = 0;
	float float_var = 0.0f;
	char string_var[256]{};
	int length = 0;
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
#endif // SMNAV_DEBUG
