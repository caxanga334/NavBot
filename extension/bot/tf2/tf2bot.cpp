#include <limits>

#include <extension.h>
#include <manager.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <util/librandom.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/tf2/tf2lib.h>
#include <mods/tf2/nav/tfnavarea.h>
#include <mods/tf2/nav/tfnav_waypoint.h>
#include <entities/tf2/tf_entities.h>
#include "tf2bot.h"

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

#undef max
#undef min
#undef clamp

#if SOURCE_ENGINE == SE_TF2
static ConVar tf2bot_change_class_allowed("sm_navbot_tf_allow_class_changes", "1", FCVAR_GAMEDLL, "Are bots allowed to change classes?");
#endif // SOURCE_ENGINE == SE_TF2

CTF2Bot::CTF2Bot(edict_t* edict) : CBaseBot(edict)
{
	m_tf2movement = std::make_unique<CTF2BotMovement>(this);
	m_tf2controller = std::make_unique<CTF2BotPlayerController>(this);
	m_tf2sensor = std::make_unique<CTF2BotSensor>(this);
	m_tf2behavior = std::make_unique<CTF2BotBehavior>(this);
	m_tf2inventory = std::make_unique<CTF2BotInventory>(this);
	m_tf2spymonitor = std::make_unique<CTF2BotSpyMonitor>(this);
	m_upgrademan.SetMe(this);
	m_cloakMeter = nullptr;
	m_doMvMUpgrade = false;
	m_classswitchtimer.Start(5.0f);
}

CTF2Bot::~CTF2Bot()
{
}

void CTF2Bot::Reset()
{
	CBaseBot::Reset();

	m_voicecmdtimer.Reset();
}

void CTF2Bot::Update()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CTF2Bot::Update", "NavBot");
#endif // EXT_VPROF_ENABLED

	CTeamFortress2Mod* mod = CTeamFortress2Mod::GetTF2Mod();

	if (mod->GetCurrentGameMode() == TeamFortress2::GameModeType::GM_MVM)
	{
		if (!m_upgrademan.IsManagerReady())
		{
			m_upgrademan.Update();
		}
	}

	if (m_doMvMUpgrade)
	{
		// refunds triggers a respawn, this triggers Reset in the middle of a task execution which results in crashes
		// to fix this, we run the manager before the base class update (which runs the behavior system).
		m_upgrademan.Update();
		m_doMvMUpgrade = false;
	}

	CBaseBot::Update();
}

void CTF2Bot::Frame()
{
	CBaseBot::Frame();
}

void CTF2Bot::TryJoinGame()
{
	JoinTeam();
	auto tfclass = CTeamFortress2Mod::GetTF2Mod()->SelectAClassForBot(this);
	JoinClass(tfclass);
	m_classswitchtimer.Start(30.0f);
}

void CTF2Bot::Spawn()
{
	FindMyBuildings();

	if (m_classswitchtimer.IsElapsed())
	{
		SelectNewClass();
		m_classswitchtimer.StartRandom(20.0f, 60.0f);
	}

	if (GetMyClassType() > TeamFortress2::TFClass_Unknown)
	{
		m_upgrademan.OnBotSpawn();
	}

	m_cloakMeter = entprops->GetPointerToEntData<float>(GetEntity(), Prop_Send, "m_flCloakMeter");

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
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CTF2Bot::GetFlagToFetch", "NavBot");
#endif // EXT_VPROF_ENABLED

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

	flag = collectedflags[randomgen->GetRandomInt<size_t>(0U, collectedflags.size() - 1U)];
	return gamehelpers->EdictOfIndex(flag);
}

edict_t* CTF2Bot::GetFlagToDefend(bool stolenOnly, bool ignoreHome) const
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CTF2Bot::GetFlagToDefend", "NavBot");
#endif // EXT_VPROF_ENABLED

	std::vector<edict_t*> collectedflags;
	collectedflags.reserve(16);
	auto myteam = GetMyTFTeam();
	const float now = gpGlobals->curtime;
	auto functor = [&collectedflags, &myteam, &stolenOnly, &ignoreHome, &now](int index, edict_t* edict, CBaseEntity* entity) {

		if (edict == nullptr)
		{
			return true; // keep loop
		}

		tfentities::HCaptureFlag flag(edict);

		if (flag.IsDisabled())
		{
			return true; // keep loop
		}

		if (flag.IsDropped())
		{
			float time;
			if (entprops->GetEntPropFloat(index, Prop_Send, "m_flResetTime", time))
			{
				if (time - now > 90.0f)
				{
					return true; // skip flags with long return time
				}
			}
		}

		if (stolenOnly && !flag.IsStolen())
		{
			return true; // keep loop
		}

		if (ignoreHome && flag.IsHome())
		{
			return true; // keep loop
		}

		if (flag.GetTFTeam() == myteam)
		{
			collectedflags.push_back(edict);
		}

		return true;
	};

	UtilHelpers::ForEachEntityOfClassname("item_teamflag", functor);

	if (collectedflags.empty())
	{
		return nullptr;
	}

	if (collectedflags.size() == 1)
	{
		return collectedflags[0];
	}

	return librandom::utils::GetRandomElementFromVector<edict_t*>(collectedflags);
}

/**
 * @brief Gets the capture zone position to deliver the flag
 * @return Capture zone position Vector
*/
edict_t* CTF2Bot::GetFlagCaptureZoreToDeliver() const
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CTF2Bot::GetFlagCaptureZoneToDeliver", "NavBot");
#endif // EXT_VPROF_ENABLED

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

CBaseEntity* CTF2Bot::GetMySentryGun() const
{
	return m_mySentryGun.Get();
}

CBaseEntity* CTF2Bot::GetMyDispenser() const
{
	return m_myDispenser.Get();
}

CBaseEntity* CTF2Bot::GetMyTeleporterEntrance() const
{
	return m_myTeleporterEntrance.Get();
}

CBaseEntity* CTF2Bot::GetMyTeleporterExit() const
{
	return m_myTeleporterExit.Get();
}

void CTF2Bot::SetMySentryGun(CBaseEntity* entity)
{
	m_mySentryGun = entity;
}

void CTF2Bot::SetMyDispenser(CBaseEntity* entity)
{
	m_myDispenser = entity;
}

void CTF2Bot::SetMyTeleporterEntrance(CBaseEntity* entity)
{
	m_myTeleporterEntrance = entity;
}

void CTF2Bot::SetMyTeleporterExit(CBaseEntity* entity)
{
	m_myTeleporterExit = entity;
}

bool CTF2Bot::IsCarryingObject() const
{
	bool carrying = false;
	entprops->GetEntPropBool(GetIndex(), Prop_Send, "m_bCarryingObject", carrying);
	return carrying;
}

CBaseEntity* CTF2Bot::GetObjectBeingCarriedByMe() const
{
	int ent = INVALID_EHANDLE_INDEX;
	entprops->GetEntPropEnt(GetIndex(), Prop_Send, "m_hCarriedObject", ent);
	return gamehelpers->ReferenceToEntity(ent);
}

void CTF2Bot::FireWeaponAtEnemy(const CKnownEntity* enemy, const bool doAim)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CTF2Bot::FireWeaponAtEnemy", "NavBot");
#endif // EXT_VPROF_ENABLED

	if (tf2lib::IsPlayerInCondition(GetEntity(), TeamFortress2::TFCond::TFCond_Taunting))
		return;

	if (!IsAlive())
		return;

	if (GetMovementInterface()->NeedsWeaponControl())
		return;

	if (!enemy->IsValid())
		return;

	const CBotWeapon* weapon = GetInventoryInterface()->GetActiveBotWeapon();

	if (!weapon || !weapon->IsValid())
		return;

	if (GetBehaviorInterface()->ShouldAttack(this, enemy) == ANSWER_NO)
		return;

	if (doAim && !GetControlInterface()->IsAimOnTarget())
		return;

	if (GetMyClassType() == TeamFortress2::TFClassType::TFClass_Heavy && GetAmmoOfIndex(TeamFortress2::TF_AMMO_PRIMARY) >= 50 &&
		GetBehaviorInterface()->ShouldHurry(this) != ANSWER_YES)
	{
		constexpr auto THREAT_SPIN_TIME = 3.0f;
		if (GetSensorInterface()->GetTimeSinceVisibleThreat() <= THREAT_SPIN_TIME)
		{
			GetControlInterface()->PressSecondaryAttackButton(0.25f); // spin the minigun
		}
	}

	const float range = GetRangeTo(enemy->GetEdict());
	bool primary = true; // primary attack by default

	if (CanFireWeapon(weapon, range, true, primary) && HandleWeapon(weapon))
	{
		if (!AimWeaponAtEnemy(enemy, weapon, doAim, range, primary))
			return;

		if (doAim && !GetControlInterface()->IsAimOnTarget())
			return;

		SetJustFiredMyWeapon(true);

		if (primary)
		{
			GetControlInterface()->PressAttackButton();
		}
		else
		{
			GetControlInterface()->PressSecondaryAttackButton();
		}
	}
	else
	{
		ReloadIfNeeded(weapon);
	}

	auto tfweapon = GetInventoryInterface()->GetActiveTFWeapon();

	// for weapons like the huntsman, release the fire button when the charge hits maximum.
	if (tfweapon && tfweapon->GetTF2Info()->CanCharge())
	{
		if (tfweapon->GetChargePercentage() >= 0.99f)
		{
			GetControlInterface()->ReleaseAllAttackButtons();
		}
	}
}

void CTF2Bot::SendVoiceCommand(TeamFortress2::VoiceCommandsID id)
{
	constexpr auto VOICE_COMMAND_COOLDOWN = 5.0f;

	if (m_voicecmdtimer.HasStarted() && m_voicecmdtimer.IsLessThen(VOICE_COMMAND_COOLDOWN))
	{
		return;
	}

	using namespace TeamFortress2;
	constexpr std::size_t size = 32U;
	std::unique_ptr<char[]> cmd = std::make_unique<char[]>(size);

	int x, y;

	if (id < VoiceCommandsID::VC_MENU1START)
	{
		x = 0;
		y = static_cast<int>(id);
	}
	else if (id >= VoiceCommandsID::VC_MENU1START && id < VoiceCommandsID::VC_MENU2START)
	{
		x = 1;
		y = static_cast<int>(id) - static_cast<int>(VoiceCommandsID::VC_MENU1START);
	}
	else /* if (id >= VoiceCommandsID::VC_MENU2START) */
	{
		x = 2;
		y = static_cast<int>(id) - static_cast<int>(VoiceCommandsID::VC_MENU2START);
	}

	ke::SafeSprintf(cmd.get(), size, "voicemenu %i %i", x, y);
	DelayedFakeClientCommand(cmd.get()); // DelayedFakeClientCommand copies the string
}

float CTF2Bot::GetTimeLeftToCapture() const
{
	CBaseEntity* pEntity = CTeamFortress2Mod::GetTF2Mod()->GetRoundTimer(tf2lib::GetEnemyTFTeam(GetMyTFTeam()));

	if (pEntity)
	{
		tfentities::HTeamRoundTimer timer(pEntity);
		
		return timer.GetTimeRemaining();
	}

	return std::numeric_limits<float>::max();
}

bool CTF2Bot::IsAbleToDodgeEnemies(const CKnownEntity* threat) const
{
	// Engineers and snipers don't dodge
	if (GetMyClassType() == TeamFortress2::TFClassType::TFClass_Engineer || GetMyClassType() == TeamFortress2::TFClassType::TFClass_Sniper)
	{
		return false;
	}

	// Don't dodge while invisible or invulnerable
	if (tf2lib::IsPlayerInvisible(GetEntity()) || tf2lib::IsPlayerInvulnerable(GetIndex()))
	{
		return false;
	}

	// Don't dodge while disguised
	if (tf2lib::IsPlayerInCondition(GetEntity(), TeamFortress2::TFCond::TFCond_Disguised))
	{
		return false;
	}

	return CBaseBot::IsAbleToDodgeEnemies(threat);
}

bool CTF2Bot::IsCarryingThePassTimeJack() const
{
	bool ret = false;
	entprops->GetEntPropBool(GetIndex(), Prop_Send, "m_bHasPasstimeBall", ret);
	return ret;
}

void CTF2Bot::SelectNewClass()
{
#if SOURCE_ENGINE == SE_TF2
	// Not allowed to change classes
	if (!tf2bot_change_class_allowed.GetBool()) 
	{
		return;
	}
#endif // SOURCE_ENGINE == SE_TF2

	// change class not possible
	if (!CTeamFortress2Mod::GetTF2Mod()->IsAllowedToChangeClasses())
	{
		return;
	}

	// Engineers: Don't switch while the sentry gun is alive
	if (GetMyClassType() == TeamFortress2::TFClassType::TFClass_Engineer && GetMySentryGun() != nullptr)
	{
		return;
	}

	// Change class if needs, or a small random chance
	if (CTeamFortress2Mod::GetTF2Mod()->ShouldSwitchClass(this) || randomgen->GetRandomInt<int>(1, 10) == 5)
	{
		TeamFortress2::TFClassType tfclass = CTeamFortress2Mod::GetTF2Mod()->SelectAClassForBot(this);

		if (TeamFortress2::IsValidTFClass(tfclass) && tfclass != GetMyClassType())
		{
			m_upgrademan.DoRefund();
			JoinClass(tfclass);
		}
	}
}

void CTF2Bot::FindMyBuildings()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CTF2Bot::FindMyBuildings", "NavBot");
#endif // EXT_VPROF_ENABLED

	m_mySentryGun.Term();
	m_myDispenser.Term();
	m_myTeleporterEntrance.Term();
	m_myTeleporterExit.Term();

	if (GetMyClassType() == TeamFortress2::TFClass_Engineer)
	{
		for (int i = gpGlobals->maxClients + 1; i < gpGlobals->maxEntities; i++)
		{
			auto edict = gamehelpers->EdictOfIndex(i);

			if (!UtilHelpers::IsValidEdict(edict))
				continue;

			CBaseEntity* pEntity = edict->GetIServerEntity()->GetBaseEntity();

			auto classname = entityprops::GetEntityClassname(pEntity);

			if (classname == nullptr || classname[0] == 0)
				continue;

			if (strncasecmp(classname, "obj_", 4) != 0)
				continue;

			tfentities::HBaseObject object(pEntity);

			// Placing means it still a blueprint
			if (object.IsPlacing())
				continue;

			if (strncasecmp(classname, "obj_sentrygun", 13) == 0)
			{
				if (object.GetBuilderIndex() == GetIndex())
				{
					SetMySentryGun(pEntity);
				}
			}
			else if (strncasecmp(classname, "obj_dispenser", 13) == 0)
			{
				if (object.GetBuilderIndex() == GetIndex())
				{
					SetMyDispenser(pEntity);
				}
			}
			else if (strncasecmp(classname, "obj_teleporter", 14) == 0)
			{
				if (object.GetBuilderIndex() == GetIndex())
				{
					if (object.GetMode() == TeamFortress2::TFObjectMode_Entrance)
					{
						SetMyTeleporterEntrance(pEntity);
					}
					else
					{
						SetMyTeleporterExit(pEntity);
					}
				}
			}
		}
	}
}

bool CTF2Bot::IsDisguised() const
{
	return tf2lib::IsPlayerInCondition(GetEntity(), TeamFortress2::TFCond_Disguised);
}

bool CTF2Bot::IsCloaked() const
{
	return tf2lib::IsPlayerInCondition(GetEntity(), TeamFortress2::TFCond_Cloaked);
}

bool CTF2Bot::IsInvisible() const
{
	return tf2lib::IsPlayerInvisible(GetEntity());
}

int CTF2Bot::GetCurrency() const
{
	int currency = 0;
	entprops->GetEntProp(GetIndex(), Prop_Send, "m_nCurrency", currency);
	return currency;
}

bool CTF2Bot::IsInUpgradeZone() const
{
	bool val = false;
	entprops->GetEntPropBool(GetIndex(), Prop_Send, "m_bInUpgradeZone", val);
	return val;
}

bool CTF2Bot::IsUsingSniperScope() const
{
	return tf2lib::IsPlayerInCondition(GetIndex(), TeamFortress2::TFCond_Zoomed);
}

void CTF2Bot::DisguiseAs(TeamFortress2::TFClassType classtype, bool myTeam)
{
	// std::unique_ptr<char[]> buffer = std::make_unique<char[]>(128);
	// ke::SafeSprintf(buffer.get(), 128, "disguise %i %i", static_cast<int>(classtype), myTeam ? -2 : -1);
	// DelayedFakeClientCommand(buffer.get());

	// disguise is a client side command, bots can't use it since they're simulated server side

	if (myTeam)
	{
		DisguiseAs(classtype, GetMyTFTeam());
	}
	else
	{
		DisguiseAs(classtype, tf2lib::GetEnemyTFTeam(GetMyTFTeam()));
	}
}

void CTF2Bot::ToggleTournamentReadyStatus(bool isready) const
{
	char command[64];
	ke::SafeSprintf(command, sizeof(command), "tournament_player_readystate %i", isready ? 1 : 0);

	// Use 'FakeClientCommand'.
	// Alternative method is manually setting the array on gamerules
	serverpluginhelpers->ClientCommand(GetEdict(), command);
}

bool CTF2Bot::TournamentIsReady() const
{
	int index = GetIndex();
	int isready = 0;
	entprops->GameRules_GetProp("m_bPlayerReady", isready, 4, index);
	return isready != 0;
}

bool CTF2Bot::IsInsideSpawnRoom() const
{
	int result = 0;
	// this should be non-zero if a bot is touching a func_respawnroom entity
	entprops->GetEntProp(GetIndex(), Prop_Send, "m_iSpawnRoomTouchCount", result);
	return result > 0;
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
	m_canblastjump = bot->GetMovementInterface()->IsAbleToBlastJump();
	m_teamID = static_cast<int>(bot->GetMyTFTeam());
}

float CTF2BotPathCost::operator()(CNavArea* toArea, CNavArea* fromArea, const CNavLadder* ladder, const NavOffMeshConnection* link, const CNavElevator* elevator, float length) const
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CTF2BotPathCost::operator()", "NavBot");
#endif // EXT_VPROF_ENABLED

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
	else if (ladder != nullptr) // experimental, very few maps have 'true' ladders
	{
		dist = ladder->m_length;
	}
	else if (elevator != nullptr)
	{
		auto fromFloor = fromArea->GetMyElevatorFloor();

		if (!fromFloor->HasCallButton() && !fromFloor->is_here)
		{
			return -1.0f; // Unable to use this elevator, lacks a button to call it to this floor and is not on this floor
		}

		dist = elevator->GetLengthBetweenFloors(fromArea, toArea);
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
	if (link == nullptr && elevator == nullptr && ladder == nullptr)
	{
		float deltaZ = fromArea->ComputeAdjacentConnectionHeightChange(toArea);

		if (deltaZ >= m_stepheight)
		{
			if (m_candoublejump)
			{
				if (deltaZ > m_maxdjheight)
				{
					// too high to reach with double jumps
					return -1.0f;
				}
			}
			else
			{
				if (deltaZ > m_maxjumpheight)
				{
					// too high to reach with regular jumps
					return -1.0f;
				}
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

		float gap = fromArea->ComputeAdjacentConnectionGapDistance(toArea);

		if (gap >= m_maxgapjumpdistance)
		{
			return -1.0f; // can't jump over this gap
		}
	}
	else if (link != nullptr)
	{
		// Don't use double jump links if we can't perform a double jump
		if (link->GetType() == OffMeshConnectionType::OFFMESH_DOUBLE_JUMP && !m_candoublejump)
		{
			return -1.0f;
		}
		else if (link->GetType() == OffMeshConnectionType::OFFMESH_BLAST_JUMP && !m_canblastjump)
		{
			return -1.0f;
		}
		else if (link->GetType() == OffMeshConnectionType::OFFMESH_JUMP_OVER_GAP)
		{
			if ((link->GetStart() - link->GetEnd()).Length() > m_maxgapjumpdistance)
			{
				return -1.0f;
			}
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

	if (m_routetype == SAFEST_ROUTE)
	{
		cost += area->GetDanger(m_teamID);
	}

	return cost;
}
