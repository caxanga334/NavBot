#include "extension.h"
#include "navmesh/nav_mesh.h"
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
			OnNavAreaChanged(m_lastnavarea, newarea);
			m_lastnavarea = newarea;
		}
	}
}

const Vector CBaseExtPlayer::GetAbsOrigin()
{
	return m_playerinfo->GetAbsOrigin();
}

const QAngle CBaseExtPlayer::GetAbsAngles()
{
	return m_playerinfo->GetAbsAngles();
}

const Vector CBaseExtPlayer::GetEyeOrigin()
{
	Vector ear;
	gameclients->ClientEarPosition(m_edict, &ear);
	return ear;
}

const QAngle CBaseExtPlayer::GetEyeAngles()
{
	// TO-DO: This is the 'RCBot' way of getting eye angles
	// Sourcemod VCalls EyeAngles.
	// Determine which is better

	auto cmd = m_playerinfo->GetLastUserCommand();
	return cmd.viewangles;
}

void CBaseExtPlayer::EyeVectors(Vector* pForward)
{
	auto eyeangles = GetEyeAngles();
	AngleVectors(eyeangles, pForward);
}

void CBaseExtPlayer::EyeVectors(Vector* pForward, Vector* pRight, Vector* pUp)
{
	auto eyeangles = GetEyeAngles();
	AngleVectors(eyeangles, pForward, pRight, pUp);
}

Vector CBaseExtPlayer::BodyDirection3D()
{
	auto angles = BodyAngles();
	Vector bodydir;
	AngleVectors(angles, &bodydir);
	return bodydir;
}

Vector CBaseExtPlayer::BodyDirection2D()
{
	auto body2d = BodyDirection3D();
	body2d.z = 0.0f;
	body2d.AsVector2D().NormalizeInPlace();
	return body2d;
}

void CBaseExtPlayer::ChangeTeam(int newTeam)
{
	m_playerinfo->ChangeTeam(newTeam);
}

int CBaseExtPlayer::GetCurrentTeamIndex()
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
#endif // SMNAV_DEBUG
