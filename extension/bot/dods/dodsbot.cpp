#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/entprops.h>
#include <mods/dods/dodslib.h>
#include <mods/dods/dayofdefeatsourcemod.h>
#include <mods/dods/nav/dods_nav_mesh.h>
#include <mods/dods/nav/dods_nav_area.h>
#include "dodsbot.h"

CDoDSBot::CDoDSBot(edict_t* edict) :
	CBaseBot(edict)
{
	m_dodsensor = std::make_unique<CDoDSBotSensor>(this);
	m_dodbehavior = std::make_unique<CDoDSBotBehavior>(this);
	m_dodinventory = std::make_unique<CDoDSBotInventory>(this);
	m_dodmovement = std::make_unique<CDoDSBotMovement>(this);
	m_dodcombat = std::make_unique<CDoDSBotCombat>(this);
	m_dodcontrol = std::make_unique<CDoDSBotPlayerController>(this);
	m_dodpathprocessor = std::make_unique<CDoDSBotPathProcessor>(this);
	m_droppedAmmo = false;
}

CDoDSBot::~CDoDSBot()
{
}

CDoDSSharedBotMemory* CDoDSBot::GetSharedMemoryInterface() const
{
	return CDayOfDefeatSourceMod::GetDODMod()->GetSharedBotMemory(GetCurrentTeamIndex());
}

void CDoDSBot::Reset()
{
	CBaseBot::Reset();
}

bool CDoDSBot::HasJoinedGame()
{
	return GetMyDoDTeam() >= dayofdefeatsource::DoDTeam::DODTEAM_ALLIES && GetMyClassType() != dayofdefeatsource::DoDClassType::DODCLASS_INVALID;
}

void CDoDSBot::TryJoinGame()
{
	DelayedFakeClientCommand("jointeam 0"); // Random Team
	DelayedFakeClientCommand("cls_random"); // Random Class
}

void CDoDSBot::Spawn()
{
	CBaseBot::Spawn();

	m_droppedAmmo = false;
	
	TryChangingClasses();
}

void CDoDSBot::FirstSpawn()
{
	CBaseBot::FirstSpawn();

	engine->SetFakeClientConVarValue(GetEdict(), "cl_autoreload", "1");
	m_nextClassChangeTimer.Start(1.0f);
}

dayofdefeatsource::DoDTeam CDoDSBot::GetMyDoDTeam() const
{
	return dodslib::GetDoDTeam(GetEntity());
}

dayofdefeatsource::DoDClassType CDoDSBot::GetMyClassType() const
{
	return dodslib::GetPlayerClassType(GetEntity());
}

int CDoDSBot::GetControlPointIndex() const
{
	int index = dayofdefeatsource::INVALID_CONTROL_POINT;
	entprops->GetEntProp(GetIndex(), Prop_Send, "m_iCPIndex", index);
	return index;
}

bool CDoDSBot::IsCapturingAPoint() const
{
	return GetControlPointIndex() != dayofdefeatsource::INVALID_CONTROL_POINT;
}

bool CDoDSBot::IsProne() const
{
	bool result = false;
	entprops->GetEntPropBool(GetIndex(), Prop_Send, "m_bProne", result);
	return result;
}

bool CDoDSBot::IsSprinting() const
{
	bool result = false;
	entprops->GetEntPropBool(GetIndex(), Prop_Send, "m_bIsSprinting", result);
	return result;
}

bool CDoDSBot::IsPlantingBomb() const
{
	bool result = false;
	entprops->GetEntPropBool(GetIndex(), Prop_Send, "m_bPlanting", result);
	return result;
}

bool CDoDSBot::IsDefusingBomb() const
{
	bool result = false;
	entprops->GetEntPropBool(GetIndex(), Prop_Send, "m_bDefusing", result);
	return result;
}

void CDoDSBot::TryChangingClasses()
{
	if (m_nextClassChangeTimer.IsElapsed())
	{
		if (IsDebugging(BOTDEBUG_MISC))
		{
			DebugPrintToConsole(255, 255, 0, "%s IS CHANGING CLASSES! \n", GetDebugIdentifier());
		}

		CDayOfDefeatSourceMod* mod = CDayOfDefeatSourceMod::GetDODMod();
		const CModSettings* settings = mod->GetModSettings();
		
		m_nextClassChangeTimer.StartRandom(settings->GetMinClassChangeTime(), settings->GetMaxClassChangeTime());

		if (!settings->IsAllowedToChangeClasses())
		{
			return;
		}

		if (!CBaseBot::s_botrng.GetRandomChance(settings->GetChangeClassChance()))
		{
			return;
		}

		auto newclass = mod->SelectClassForBot(this);

		if (newclass != dayofdefeatsource::DoDClassType::DODCLASS_INVALID)
		{
			DelayedFakeClientCommand(dodslib::GetJoinClassCommand(newclass, GetMyDoDTeam()));
		}
	}
}

CDoDSBotPathCost::CDoDSBotPathCost(CDoDSBot* bot, RouteType type) :
	IGroundPathCost(bot)
{
	m_me = bot;
	m_dodpathprocessor = bot->GetPathProcessorInterface();
	m_routetype = type;
	m_hasbomb = bot->GetInventoryInterface()->HasBomb();
}

float CDoDSBotPathCost::operator()(CNavArea* baseToArea, CNavArea* fromArea, const CNavLadder* ladder, const NavOffMeshConnection* link, const CNavElevator* elevator, float length) const
{
	float cost = GetGroundMovementCost(baseToArea, fromArea, ladder, link, elevator, length);
	cost = ApplyCostModifiers(m_me, baseToArea, cost);

	if (cost < 0.0f)
	{
		return -1.0f; // base reports dead end
	}

	CDoDSNavArea* toArea = static_cast<CDoDSNavArea*>(baseToArea);

	// Area is blocked if we don't have bombs
	if (toArea->HasDoDAttributes(CDoDSNavArea::DoDNavAttributes::DODNAV_BOMBS_TO_OPEN) && !toArea->WasBombed())
	{
		if (!toArea->CanMyTeamPlantBomb(m_teamindex))
		{
			return -1.0f; // I can't open this path, dead end
		}

		m_dodpathprocessor->OnAreaBlockedByBomb(baseToArea);

		if (!m_hasbomb)
		{
			// Soft block this path by returning a very high cost
			return 1e12f;
		}
	}

	return cost;
}
