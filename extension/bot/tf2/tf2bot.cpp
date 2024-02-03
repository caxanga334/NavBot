#include <extension.h>
#include <manager.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <util/librandom.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <mods/tf2/nav/tfnavarea.h>
#include <entities/tf2/tf_entities.h>
#include "tf2bot.h"

#undef max
#undef min
#undef clamp

CTF2Bot::CTF2Bot(edict_t* edict) : CBaseBot(edict)
{
	m_tf2movement = std::make_unique<CTF2BotMovement>(this);
	m_tf2controller = std::make_unique<CTF2BotPlayerController>(this);
	m_tf2sensor = std::make_unique<CTF2BotSensor>(this);
	m_tf2behavior = std::make_unique<CTF2BotBehavior>(this);
	m_desiredclass = TeamFortress2::TFClassType::TFClass_Unknown;
}

CTF2Bot::~CTF2Bot()
{
}

void CTF2Bot::TryJoinGame()
{
	JoinTeam();
	auto tfclass = CTeamFortress2Mod::GetTF2Mod()->SelectAClassForBot(this);
	JoinClass(tfclass);
}

void CTF2Bot::Spawn()
{
	CBaseBot::Spawn();
}

void CTF2Bot::FirstSpawn()
{
	CBaseBot::FirstSpawn();
}

int CTF2Bot::GetMaxHealth() const
{
	return tf2lib::GetPlayerMaxHealth(GetIndex());
}

TeamFortress2::TFClassType CTF2Bot::GetMyClassType() const
{
	return tf2lib::GetPlayerClassType(GetIndex());
}

TeamFortress2::TFTeam CTF2Bot::GetMyTFTeam() const
{
	return tf2lib::GetEntityTFTeam(GetIndex());
}

void CTF2Bot::JoinClass(TeamFortress2::TFClassType tfclass) const
{
	auto szclass = tf2lib::GetClassNameFromType(tfclass);
	char command[128];
	ke::SafeSprintf(command, sizeof(command), "joinclass %s", szclass);
	FakeClientCommand(command);
}

void CTF2Bot::JoinTeam() const
{
	FakeClientCommand("jointeam auto");
}

edict_t* CTF2Bot::GetItem() const
{
	edict_t* item = nullptr;
	int entity = -1;

	if (entprops->GetEntPropEnt(GetIndex(), Prop_Send, "m_hItem", entity))
	{
		UtilHelpers::IndexToAThings(entity, nullptr, &item);
	}

	return item;
}

bool CTF2Bot::IsCarryingAFlag() const
{
	auto item = GetItem();

	if (!item)
		return false;

	if (strncasecmp(gamehelpers->GetEntityClassname(item), "item_teamflag", 13) == 0)
	{
		return true;
	}

	return false;
}

edict_t* CTF2Bot::GetFlagToFetch() const
{
	std::vector<int> collectedflags;
	collectedflags.reserve(16);

	if (IsCarryingAFlag())
		return GetItem();

	int flag = INVALID_ENT_REFERENCE;

	while ((flag = UtilHelpers::FindEntityByClassname(flag, "item_teamflag")) != INVALID_ENT_REFERENCE)
	{
		CBaseEntity* pFlag = nullptr;
		
		if (!UtilHelpers::IndexToAThings(flag, &pFlag, nullptr))
			continue;

		tfentities::HCaptureFlag eFlag(pFlag);

		if (eFlag.IsDisabled())
			continue;

		if (eFlag.IsStolen())
			continue; // ignore stolen flags

		switch (eFlag.GetType())
		{
		case TeamFortress2::TFFlagType::TF_FLAGTYPE_CTF:
			if (eFlag.GetTFTeam() == tf2lib::GetEnemyTFTeam(GetMyTFTeam()))
			{
				collectedflags.push_back(flag);
			}
			break;
		case TeamFortress2::TFFlagType::TF_FLAGTYPE_ATTACK_DEFEND:
		case TeamFortress2::TFFlagType::TF_FLAGTYPE_TERRITORY_CONTROL:
		case TeamFortress2::TFFlagType::TF_FLAGTYPE_INVADE:
			if (eFlag.GetTFTeam() != tf2lib::GetEnemyTFTeam(GetMyTFTeam()))
			{
				collectedflags.push_back(flag);
			}
			break;
		default:
			break;
		}
	}

	if (collectedflags.size() == 0)
		return nullptr;

	if (collectedflags.size() == 1)
	{
		flag = collectedflags.front();
		return gamehelpers->EdictOfIndex(flag);
	}

	flag = collectedflags[librandom::generate_random_uint(0, collectedflags.size() - 1)];
	return gamehelpers->EdictOfIndex(flag);
}

/**
 * @brief Gets the capture zone position to deliver the flag
 * @return Capture zone position Vector
*/
edict_t* CTF2Bot::GetFlagCaptureZoreToDeliver() const
{
	int capturezone = INVALID_ENT_REFERENCE;

	while ((capturezone = UtilHelpers::FindEntityByClassname(capturezone, "func_capturezone")) != INVALID_ENT_REFERENCE)
	{
		CBaseEntity* pZone = nullptr;
		edict_t* edict = nullptr;

		if (!UtilHelpers::IndexToAThings(capturezone, &pZone, &edict))
			continue;

		tfentities::HCaptureZone entity(pZone);

		if (entity.IsDisabled())
			continue;

		if (entity.GetTFTeam() != GetMyTFTeam())
			continue;

		return edict; // return the first found
	}

	return nullptr;
}

CTF2BotPathCost::CTF2BotPathCost(CTF2Bot* bot, RouteType routetype)
{
	m_me = bot;
	m_routetype = routetype;
	m_stepheight = bot->GetMovementInterface()->GetStepHeight();
	m_maxjumpheight = bot->GetMovementInterface()->GetMaxJumpHeight();
	m_maxdropheight = bot->GetMovementInterface()->GetMaxDropHeight();
}

float CTF2BotPathCost::operator()(CNavArea* toArea, CNavArea* fromArea, const CNavLadder* ladder, const CFuncElevator* elevator, float length)
{
	if (fromArea == nullptr)
	{
		// first area in path, no cost
		return 0.0f;
	}

	CTFNavArea* area = static_cast<CTFNavArea*>(toArea);

	if (!m_me->GetMovementInterface()->IsAreaTraversable(area))
	{
		return -1.0f;
	}

	float dist = 0.0f;

	if (length > 0.0f)
	{
		dist = length;
	}
	else
	{
		dist = (toArea->GetCenter() + fromArea->GetCenter()).Length();
	}

	float deltaZ = fromArea->ComputeAdjacentConnectionHeightChange(area);

	if (deltaZ >= m_stepheight)
	{
		if (deltaZ >= m_maxjumpheight)
		{
			// too high to reach
			return -1.0f;
		}

		// add jump penalty
		constexpr auto jumpPenalty = 2.0f;
		dist *= jumpPenalty;
	}
	else if (deltaZ < -m_maxdropheight)
	{
		// too far to drop
		return -1.0f;
	}

	if (area->HasTFPathAttributes(CTFNavArea::TFNAV_PATH_NO_CARRIERS))
	{
		if (m_me->IsCarryingAFlag())
		{
			return -1.0f;
		}
	}

	if (area->HasTFPathAttributes(CTFNavArea::TFNAV_PATH_CARRIERS_AVOID))
	{
		if (m_me->IsCarryingAFlag())
		{
			constexpr auto pathflag_carrier_avoid = 7.0f;
			dist *= pathflag_carrier_avoid;
		}
	}

	float cost = dist + fromArea->GetCostSoFar();

	return cost;
}
