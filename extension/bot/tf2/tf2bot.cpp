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

static ConVar sm_navbot_tf_medic_patient_scan_range("sm_navbot_tf_medic_patient_scan_range", "1500", FCVAR_GAMEDLL, "Medic AI: Distance to search for patients to heal.", true, 512.0f, true, 16384.0f);

CTF2Bot::CTF2Bot(edict_t* edict) : CBaseBot(edict)
{
	m_tf2movement = std::make_unique<CTF2BotMovement>(this);
	m_tf2controller = std::make_unique<CTF2BotPlayerController>(this);
	m_tf2sensor = std::make_unique<CTF2BotSensor>(this);
	m_tf2behavior = std::make_unique<CTF2BotBehavior>(this);
	m_desiredclass = TeamFortress2::TFClassType::TFClass_Unknown;
	m_knownspythink = TIME_TO_TICKS(knownspy_think_interval());
}

CTF2Bot::~CTF2Bot()
{
}

void CTF2Bot::Reset()
{
	m_knownspies.clear();
	m_knownspythink = TIME_TO_TICKS(knownspy_think_interval());
	CBaseBot::Reset();
}

void CTF2Bot::Frame()
{
	if (--m_knownspythink < 0)
	{
		UpdateKnownSpies();
		m_knownspythink = TIME_TO_TICKS(knownspy_think_interval());
	}

	CBaseBot::Frame();
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

	FindMyBuildings();
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

	if (item == nullptr)
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

	int flag = INVALID_EHANDLE_INDEX;

	while ((flag = UtilHelpers::FindEntityByClassname(flag, "item_teamflag")) != INVALID_EHANDLE_INDEX)
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
	int capturezone = INVALID_EHANDLE_INDEX;

	while ((capturezone = UtilHelpers::FindEntityByClassname(capturezone, "func_capturezone")) != INVALID_EHANDLE_INDEX)
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

bool CTF2Bot::IsAmmoLow() const
{
	// For engineer, check metal
	if (GetMyClassType() == TeamFortress2::TFClass_Engineer)
	{
		if (GetAmmoOfIndex(TeamFortress2::TF_AMMO_METAL) <= 0)
		{
			return true;
		}
	}

	bool haslowammoweapon = false;

	ForEveryWeapon([this, &haslowammoweapon](const CBotWeapon& weapon) {
		if (haslowammoweapon)
			return;

		if (!weapon.GetWeaponInfo().IsCombatWeapon())
		{
			return; // don't bother with ammo for non combat weapons
		}

		if (weapon.GetWeaponInfo().HasLowPrimaryAmmoThreshold())
		{
			if (GetAmmoOfIndex(weapon.GetBaseCombatWeapon().GetPrimaryAmmoType()) < weapon.GetWeaponInfo().GetLowPrimaryAmmoThreshold())
			{
				haslowammoweapon = true;
				return;
			}
		}

		if (weapon.GetWeaponInfo().HasLowSecondaryAmmoThreshold())
		{
			if (GetAmmoOfIndex(weapon.GetBaseCombatWeapon().GetSecondaryAmmoType()) < weapon.GetWeaponInfo().GetLowSecondaryAmmoThreshold())
			{
				haslowammoweapon = true;
				return;
			}
		}
	});

	return haslowammoweapon;
}

edict_t* CTF2Bot::MedicFindBestPatient() const
{
	const float max_scan_range = sm_navbot_tf_medic_patient_scan_range.GetFloat();

	// filter for getting potential teammates to heal
	auto functor = [this, &max_scan_range](int client, edict_t* entity, SourceMod::IGamePlayer* player) -> bool {
		auto myteam = GetMyTFTeam();
		auto theirteam = tf2lib::GetEntityTFTeam(client);

		if (client == GetIndex())
		{
			return false; // can't heal self
		}

		if (tf2lib::GetPlayerClassType(client) == TeamFortress2::TFClass_Spy && tf2lib::IsPlayerDisguised(client))
		{
			theirteam = tf2lib::GetDisguiseTeam(client);
		}

		if (myteam != theirteam) // can only heal teammates
		{
			return false;
		}

		if (tf2lib::IsPlayerInvisible(client))
		{
			return false; // can't heal invisible players
		}

		if (!UtilHelpers::IsPlayerAlive(client))
		{
			return false;
		}

		Vector mypos = WorldSpaceCenter();
		Vector theirpos = UtilHelpers::getWorldSpaceCenter(entity);
		float range = (theirpos - mypos).Length();
		

		if (range > max_scan_range)
		{
			return false;
		}

		// Don't do LOS checks, medic has an autocall feature that can be used to find teammates

		return true;
	};

	std::vector<int> players;
	players.reserve(100);
	UtilHelpers::CollectPlayers(players, functor);

	if (players.size() == 0)
	{
		return nullptr; // no player to heal
	}

	edict_t* best = nullptr;
	float bestrange = std::numeric_limits<float>::max();
	Vector mypos = WorldSpaceCenter();

	for (auto& client : players)
	{
		auto them = gamehelpers->EdictOfIndex(client);
		Vector theirpos = UtilHelpers::getWorldSpaceCenter(them);
		float distance = (theirpos - mypos).Length();
		float health_percent = tf2lib::GetPlayerHealthPercentage(client);

		if (health_percent >= 1.0f)
		{
			distance *= 1.25f; // at max health or overhealed
		}

		if (health_percent <= medic_patient_health_critical_level())
		{
			distance *= 0.5f;
		}

		if (health_percent <= medic_patient_health_low_level())
		{
			distance *= 0.75f;
		}

		if (tf2lib::IsPlayerInCondition(client, TeamFortress2::TFCond_OnFire) ||
			tf2lib::IsPlayerInCondition(client, TeamFortress2::TFCond_Bleeding))
		{
			distance *= 0.8f;
		}

		switch (tf2lib::GetPlayerClassType(client))
		{
		case TeamFortress2::TFClass_DemoMan:
		case TeamFortress2::TFClass_Soldier:
		case TeamFortress2::TFClass_Heavy:
		case TeamFortress2::TFClass_Medic:
			distance *= 0.6f; // big preference for these classes
			break;
		case TeamFortress2::TFClass_Scout:
		case TeamFortress2::TFClass_Sniper:
			distance *= 1.3f; // low priority for these classes
			break;
		default:
			break;
		}

		if (distance < bestrange)
		{
			bestrange = distance;
			best = them;
		}
	}

	if (best != nullptr)
	{
		DebugPrintToConsole(BOTDEBUG_TASKS, 0, 102, 0, "%s MEDIC: Found patient %s, range factor: %3.4f \n", GetDebugIdentifier(), 
			UtilHelpers::GetPlayerDebugIdentifier(best), bestrange);
	}

	return best;
}

edict_t* CTF2Bot::GetMySentryGun() const
{
	return UtilHelpers::GetEdictFromCBaseHandle(m_mySentryGun);
}

edict_t* CTF2Bot::GetMyDispenser() const
{
	return UtilHelpers::GetEdictFromCBaseHandle(m_myDispenser);
}

edict_t* CTF2Bot::GetMyTeleporterEntrance() const
{
	return UtilHelpers::GetEdictFromCBaseHandle(m_myTeleporterEntrance);
}

edict_t* CTF2Bot::GetMyTeleporterExit() const
{
	return UtilHelpers::GetEdictFromCBaseHandle(m_myTeleporterExit);
}

void CTF2Bot::SetMySentryGun(edict_t* entity)
{
	UtilHelpers::SetHandleEntity(m_mySentryGun, entity);
}

void CTF2Bot::SetMyDispenser(edict_t* entity)
{
	UtilHelpers::SetHandleEntity(m_myDispenser, entity);
}

void CTF2Bot::SetMyTeleporterEntrance(edict_t* entity)
{
	UtilHelpers::SetHandleEntity(m_myTeleporterEntrance, entity);
}

void CTF2Bot::SetMyTeleporterExit(edict_t* entity)
{
	UtilHelpers::SetHandleEntity(m_myTeleporterExit, entity);
}

void CTF2Bot::FindMyBuildings()
{
	m_mySentryGun.Term();
	m_myDispenser.Term();
	m_myTeleporterEntrance.Term();
	m_myTeleporterExit.Term();

	if (GetMyClassType() == TeamFortress2::TFClass_Engineer)
	{
		for (int i = gpGlobals->maxClients + 1; i < gpGlobals->maxEntities; i++)
		{
			auto edict = gamehelpers->EdictOfIndex(i);

			if (edict == nullptr || edict->IsFree())
				continue;

			auto classname = gamehelpers->GetEntityClassname(edict);

			if (classname == nullptr || classname[0] == 0)
				continue;

			tfentities::HBaseObject object(edict);

			if (strncasecmp(classname, "obj_sentrygun", 13) == 0)
			{
				if (object.GetBuilderIndex() == GetIndex())
				{
					SetMySentryGun(edict);
				}
			}
			else if (strncasecmp(classname, "obj_dispenser", 13) == 0)
			{
				if (object.GetBuilderIndex() == GetIndex())
				{
					SetMyDispenser(edict);
				}
			}
			else if (strncasecmp(classname, "obj_teleporter", 14) == 0)
			{
				if (object.GetBuilderIndex() == GetIndex())
				{
					if (object.GetMode() == TeamFortress2::TFObjectMode_Entrance)
					{
						SetMyTeleporterEntrance(edict);
					}
					else
					{
						SetMyTeleporterExit(edict);
					}
				}
			}
		}
	}
}

const CTF2Bot::KnownSpy* CTF2Bot::GetKnownSpy(edict_t* spy) const
{
	int index = gamehelpers->IndexOfEdict(spy);

	for (const auto& knownspy : m_knownspies)
	{
		if (!knownspy.IsValid())
			continue;

		if (knownspy.GetIndex() == index)
		{
			return &knownspy;
		}
	}

	return nullptr;
}

const CTF2Bot::KnownSpy* CTF2Bot::UpdateOrCreateKnownSpy(edict_t* spy, KnownSpyInfo info, const bool updateinfo, const bool updateclass)
{
	int index = gamehelpers->IndexOfEdict(spy);

	for (auto& knownspy : m_knownspies)
	{
		if (!knownspy.IsValid())
			continue;

		if (knownspy.GetIndex() == index)
		{
			knownspy.Update(info, updateinfo, updateclass);
			return &knownspy;
		}
	}

	// if we reach here, the spy is not in the known list
	auto& newknown = m_knownspies.emplace_back(spy, info);
	return &newknown;
}

CTF2BotPathCost::CTF2BotPathCost(CTF2Bot* bot, RouteType routetype)
{
	m_me = bot;
	m_routetype = routetype;
	m_stepheight = bot->GetMovementInterface()->GetStepHeight();
	m_maxjumpheight = bot->GetMovementInterface()->GetMaxJumpHeight();
	m_maxdropheight = bot->GetMovementInterface()->GetMaxDropHeight();
	m_maxdjheight = bot->GetMovementInterface()->GetMaxDoubleJumpHeight();
	m_maxgapjumpdistance = bot->GetMovementInterface()->GetMaxGapJumpDistance();
	m_candoublejump = bot->GetMovementInterface()->IsAbleToDoubleJump();
}

float CTF2BotPathCost::operator()(CNavArea* toArea, CNavArea* fromArea, const CNavLadder* ladder, const NavSpecialLink* link, const CFuncElevator* elevator, float length) const
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

	if (link != nullptr)
	{
		dist = link->GetConnectionLength();
	}
	else if (length > 0.0f)
	{
		dist = length;
	}
	else
	{
		dist = (toArea->GetCenter() + fromArea->GetCenter()).Length();
	}

	// only check gap and height on common connections
	if (link == nullptr)
	{
		float deltaZ = fromArea->ComputeAdjacentConnectionHeightChange(area);

		if (deltaZ >= m_stepheight)
		{
			if (deltaZ >= m_maxjumpheight && !m_candoublejump) // can't double jump
			{
				// too high to reach
				return -1.0f;
			}

			if (deltaZ >= m_maxdjheight) // can double jump
			{
				// too high to reach
				return -1.0f;
			}

			// jump type is resolved by the navigator

			// add jump penalty
			constexpr auto jumpPenalty = 2.0f;
			dist *= jumpPenalty;
		}
		else if (deltaZ < -m_maxdropheight)
		{
			// too far to drop
			return -1.0f;
		}

		float gap = fromArea->ComputeAdjacentConnectionGapDistance(area);

		if (gap >= m_maxgapjumpdistance)
		{
			return -1.0f; // can't jump over this gap
		}
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

CTF2Bot::KnownSpy::KnownSpy(edict_t* spy, KnownSpyInfo info)
{
	m_handle.Set(spy->GetIServerEntity());
	m_info = info;
	m_time.Start();
	
	if (tf2lib::IsPlayerInCondition(m_handle.GetEntryIndex(), TeamFortress2::TFCond_Disguised))
	{
		m_lastclass = tf2lib::GetDisguiseClass(m_handle.GetEntryIndex());
	}
	else
	{
		m_lastclass = TeamFortress2::TFClass_Unknown;
	}
}

void CTF2Bot::KnownSpy::Update(KnownSpyInfo newinfo, const bool updateinfo, const bool updateclass)
{
	m_time.Start();

	if (updateinfo)
	{
		m_info = newinfo;
	}

	if (updateclass && tf2lib::IsPlayerDisguised(m_handle.GetEntryIndex()))
	{
		// if disguised, update last disguise class, otherwise keep the original in memory
		m_lastclass = tf2lib::GetDisguiseClass(m_handle.GetEntryIndex());
	}
}

bool CTF2Bot::KnownSpy::IsValid() const
{
	return m_handle.IsValid() && (UtilHelpers::GetEdictFromCBaseHandle(m_handle) != nullptr);
}

edict_t* CTF2Bot::KnownSpy::GetEdict() const
{
	return UtilHelpers::GetEdictFromCBaseHandle(m_handle);
}

bool CTF2Bot::KnownSpy::IsSuspicious(CTF2Bot* me) const
{
	if (m_info == KNOWNSPY_FOUND)
	{
		return true; // Known spy
	}

	if (tf2lib::GetDisguiseTeam(m_handle.GetEntryIndex()) != me->GetMyTFTeam())
	{
		return true; // not disguised as my team
	}

	auto currentdisguiseclass = tf2lib::GetDisguiseClass(m_handle.GetEntryIndex());
	auto currentdisguisetarget = tf2lib::GetDisguiseTarget(m_handle.GetEntryIndex());
	float range = me->GetRangeTo(GetEdict());

	if (m_info == KNOWNSPY_SUSPICIOUS)
	{
		if (currentdisguiseclass == m_lastclass)
		{
			return true; // detect suspicious spies that didn't change their disguise class
		}

		if (range <= sus_too_close_range())
		{
			return true; // suspicious spy got too close for comfort
		}
	}

	if (currentdisguisetarget != nullptr)
	{
		int targetindex = gamehelpers->IndexOfEdict(currentdisguisetarget);

		if (targetindex == me->GetIndex())
		{
			return true; // disguised as me!
		}

		if (!UtilHelpers::IsPlayerAlive(targetindex))
		{
			return true; // disguised as a dead teammate
		}

		auto theirclass = tf2lib::GetPlayerClassType(targetindex);

		if (currentdisguiseclass != theirclass)
		{
			return true; // disguise class and disguise target class mismatch
		}
	}

	// Too close
	if (range <= touch_range())
	{
		return true;
	}

	return false;
}
