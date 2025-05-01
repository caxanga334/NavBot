#include <extension.h>
#include <manager.h>
#include <extplayer.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <util/sdkcalls.h>
#include <mods/basemod.h>
#include <tier1/convar.h>
#include <sdkports/sdk_takedamageinfo.h>
#include <sdkports/sdk_traces.h>
#include "basebot_behavior.h"
#include "basebot.h"

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

ConVar cvar_bot_difficulty("sm_navbot_skill_level", "0", FCVAR_NONE, "Skill level group to use when selecting bot difficulty.");
ConVar cvar_bot_disable_behavior("sm_navbot_debug_disable_gamemode_ai", "0", FCVAR_CHEAT | FCVAR_GAMEDLL, "When set to 1, disables game mode behavior for the bot AI.");

CBaseBot::CBaseBot(edict_t* edict) : CBaseExtPlayer(edict),
	m_cmd(),
	m_viewangles(0.0f, 0.0f, 0.0f)
{
	m_spawnTime = -1.0f;
	m_simulationtick = -1;
	m_profile = extmanager->GetMod()->GetBotDifficultyManager()->GetProfileForSkillLevel(cvar_bot_difficulty.GetInt());
	m_isfirstspawn = false;
	m_justfiredmyweapon = false;
	m_nextupdatetime.Invalidate();
	m_joingametime = 64;
	m_controller = nullptr; // Because the bot is now allocated at 'OnClientPutInServer' no bot controller was created yet.
	m_listeners.reserve(8);
	m_basecontrol = nullptr;
	m_basemover = nullptr;
	m_basesensor = nullptr;
	m_basebehavior = nullptr;
	m_baseinventory = nullptr;
	m_weaponselect = 0;
	m_cmdtimer.Invalidate();
	m_cmdsents = 0;
	m_debugtextoffset = 0;
	m_shhooks.reserve(32);
	m_impulse = 0;
	m_lastPrerequisite = nullptr;

	AddHooks();
}

CBaseBot::~CBaseBot()
{
	m_interfaces.clear();
	m_listeners.clear();

	for (auto& hook : m_shhooks)
	{
		SH_REMOVE_HOOK_ID(hook);
	}

	m_shhooks.clear();
}

void CBaseBot::PostAdd()
{
	if (m_controller == nullptr)
	{
		m_controller = botmanager->GetBotController(GetEdict());
	}

#ifdef EXT_DEBUG
	ConColorMsg(Color(0, 255, 60, 255), "CBaseBot::PostAdd m_controller = %p \n", m_controller);
#endif // EXT_DEBUG
}

float CBaseBot::GetTimeSinceLastSpawn() const
{
	return gpGlobals->curtime - m_spawnTime;
}

std::vector<IEventListener*>* CBaseBot::GetListenerVector()
{
	return &m_listeners;
}

void CBaseBot::PlayerThink()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CBaseBot::PlayerThink", "NavBot");
#endif // EXT_VPROF_ENABLED

	CBaseExtPlayer::PlayerThink(); // Call base class function first

	m_debugtextoffset = 0;

	if (IsDebugging(BOTDEBUG_TASKS))
	{
		DebugFrame();
	}

	// this needs to be before the sleep check since spectator players are dead
	// bots created by plugins will have to be handled by the 
	if (--m_joingametime <= 0)
	{
		if (!HasJoinedGame() && CanJoinGame())
		{
			TryJoinGame();
			m_joingametime = TIME_TO_TICKS(1.0f);
		}
		else if (HasJoinedGame())
		{
			m_joingametime = TIME_TO_TICKS(20.0f); // if the bot is in-game already, run this check every 20 seconds
		}
	}

	if (SleepWhenDead() && GetPlayerInfo()->IsDead())
	{
		int deadbuttons = 0;
		DeadSleepThink(deadbuttons);
		BuildUserCommand(deadbuttons); // Still send empty usercommands while dead
		return;
	}

	// Clear the move weight from the last frame
	GetMovementInterface()->ClearMoveWeight();

	Frame(); // Call bot frame

	int buttons = 0;
	auto control = GetControlInterface();

	if (m_nextupdatetime.IsElapsed())
	{
		m_nextupdatetime.Start(extmanager->GetMod()->GetModSettings()->GetUpdateRate());

		Update(); // Run period update

		// Process all buttons during updates
		control->ProcessButtons(buttons);
	}
	else // Not running a full update
	{
		// Keep last cmd buttons pressed + any new button pressed
		buttons = control->GetOldButtonsToSend();
	}

	// This needs to be the last call on a bot think cycle
	BuildUserCommand(buttons);
	control->OnPostRunCommand();
	// Execute the command at the end of the tick since the BOT AI might queue commands on this tick
	ExecuteQueuedCommands(); // Run queue commands
}

void CBaseBot::NavAreaChanged(CNavArea* old, CNavArea* current)
{
	/* 
	 * Called by the base class when updating the last known nav area 
	 * Propagate the event to the behavior
	*/

	this->OnNavAreaChanged(old, current);
}

void CBaseBot::RefreshDifficulty(const CDifficultyManager* manager)
{
	m_profile = manager->GetProfileForSkillLevel(cvar_bot_difficulty.GetInt());

	for (auto iface : m_interfaces)
	{
		iface->OnDifficultyProfileChanged();
	}
}

void CBaseBot::Reset()
{
	m_nextupdatetime.Invalidate();
	m_reloadCheckDelay.Invalidate();
	m_lastPrerequisite = nullptr;
	m_clearLastPrerequisiteTimer.Invalidate();

	for (auto iface : m_interfaces)
	{
		iface->Reset();
	}
}

void CBaseBot::Update()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CBaseBot::Update", "NavBot");
#endif // EXT_VPROF_ENABLED

	if (m_clearLastPrerequisiteTimer.HasStarted() && m_clearLastPrerequisiteTimer.IsElapsed())
	{
		m_clearLastPrerequisiteTimer.Invalidate();
		m_lastPrerequisite = nullptr;
	}

	for (auto iface : m_interfaces)
	{
		iface->Update();
	}
}

void CBaseBot::Frame()
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CBaseBot::Frame", "NavBot");
#endif // EXT_VPROF_ENABLED

	for (auto iface : m_interfaces)
	{
		iface->Frame();
	}
}

float CBaseBot::GetRangeTo(const Vector& pos) const
{
	return (GetAbsOrigin() - pos).Length();
}

float CBaseBot::GetRangeTo(edict_t* edict) const
{
	return (GetAbsOrigin() - edict->GetCollideable()->GetCollisionOrigin()).Length();
}

float CBaseBot::GetRangeTo(CBaseEntity* entity) const
{
	return (GetAbsOrigin() - reinterpret_cast<IServerEntity*>(entity)->GetCollideable()->GetCollisionOrigin()).Length();
}

float CBaseBot::GetRangeToSqr(const Vector& pos) const
{
	return (GetAbsOrigin() - pos).LengthSqr();
}

float CBaseBot::GetRangeToSqr(edict_t* edict) const
{
	return (GetAbsOrigin() - edict->GetCollideable()->GetCollisionOrigin()).LengthSqr();
}

float CBaseBot::GetRangeToSqr(CBaseEntity* entity) const
{
	return (GetAbsOrigin() - reinterpret_cast<IServerEntity*>(entity)->GetCollideable()->GetCollisionOrigin()).LengthSqr();
}

bool CBaseBot::IsRangeGreaterThan(const Vector& pos, const float range) const
{
	Vector to = (pos - GetAbsOrigin());
	return to.Length() > range;
}

bool CBaseBot::IsRangeGreaterThan(edict_t* edict, const float range) const
{
	Vector to = (edict->GetCollideable()->GetCollisionOrigin() - GetAbsOrigin());
	return to.Length() > range;
}

bool CBaseBot::IsRangeGreaterThan(CBaseEntity* entity, const float range) const
{
	Vector to = (GetAbsOrigin() - reinterpret_cast<IServerEntity*>(entity)->GetCollideable()->GetCollisionOrigin());
	return to.Length() > range;
}

bool CBaseBot::IsRangeLessThan(const Vector& pos, const float range) const
{
	Vector to = (pos - GetAbsOrigin());
	return to.Length() < range;
}

bool CBaseBot::IsRangeLessThan(edict_t* edict, const float range) const
{
	Vector to = (edict->GetCollideable()->GetCollisionOrigin() - GetAbsOrigin());
	return to.Length() < range;
}

bool CBaseBot::IsRangeLessThan(CBaseEntity* entity, const float range) const
{
	Vector to = (GetAbsOrigin() - reinterpret_cast<IServerEntity*>(entity)->GetCollideable()->GetCollisionOrigin());
	return to.Length() < range;
}

/**
 * @brief Checks if the bot is able to break the given entity
 * @param entity Entity the bot needs to break
 * @return true if the bot can break this entity
*/
bool CBaseBot::IsAbleToBreak(edict_t* entity)
{
	int index = gamehelpers->IndexOfEdict(entity);

	if (index == -1)
	{
		return false;
	}

	int takedamage = 0;

	if (entprops->GetEntProp(index, Prop_Data, "m_takedamage", takedamage) == true)
	{
		switch (takedamage)
		{
		case DAMAGE_NO:
			[[fallthrough]];
		case DAMAGE_EVENTS_ONLY:
		{
			return false; // entity doesn't take damage
		}
		default:
			break;
		}
	}

	int health = 0;
	constexpr auto MAX_HEALTH_TO_BREAK = 1000; // if the entity health is greater than this, don't bother trying to break it
	
	if (entprops->GetEntProp(index, Prop_Data, "m_iHealth", health) == true)
	{
		if (health > MAX_HEALTH_TO_BREAK)
		{
			return false; // don't bother
		}
	}

	auto classname = gamehelpers->GetEntityClassname(entity);

	if (strncmp(classname, "func_breakable", 14) == 0)
	{
		return true;
	}

	if (strncmp(classname, "func_breakable_surf", 19) == 0)
	{
		return true;
	}

	return false;
}

bool CBaseBot::IsAlive() const
{
	return UtilHelpers::IsEntityAlive(this->GetEntity());
}

void CBaseBot::RegisterInterface(IBotInterface* iface)
{
	m_interfaces.push_back(iface);
	m_listeners.push_back(iface);
}

void CBaseBot::BuildUserCommand(const int buttons)
{
	if (!RunPlayerCommands())
	{
		return;
	}

	int commandnumber = m_cmd.command_number;
	auto mover = GetMovementInterface();
	auto control = GetControlInterface();

	m_cmd.Reset();

	commandnumber++;
	m_cmd.command_number = commandnumber;
	m_cmd.tick_count = gpGlobals->tickcount;
	m_cmd.impulse = static_cast<byte>(m_impulse);
	m_cmd.viewangles = m_viewangles; // set view angles
	m_cmd.weaponselect = m_weaponselect; // send weapon select
	// TO-DO: weaponsubtype
	m_cmd.buttons = buttons; // send buttons
	m_cmd.random_seed = CBaseBot::s_usercmdrng.GetRandomInt<int>(0, 0x7fffffff);

	float forwardspeed = 0.0f;
	float sidespeed = 0.0f;
	const float desiredspeed = mover->GetDesiredSpeed();

	if ((buttons & INPUT_FORWARD) != 0)
	{
		forwardspeed = desiredspeed;
	}
	else if ((buttons & INPUT_BACK) != 0)
	{
		forwardspeed = -desiredspeed;
	}

	if ((buttons & INPUT_MOVERIGHT) != 0)
	{
		sidespeed = desiredspeed;
	}
	else if ((buttons & INPUT_MOVELEFT) != 0)
	{
		sidespeed = -desiredspeed;
	}

	// TO-DO: Up speed

	if (control->ShouldApplyScale())
	{
		forwardspeed = forwardspeed * control->GetForwardScale();
		sidespeed = sidespeed * control->GetSideScale();
	}

	m_cmd.forwardmove = forwardspeed;
	m_cmd.sidemove = sidespeed;
	m_cmd.upmove = 0.0f;

	RunUserCommand(&m_cmd);

	m_weaponselect = 0;
	m_impulse = 0;
}

void CBaseBot::RunUserCommand(CBotCmd* ucmd)
{
	if (!extension->ShouldCallRunPlayerCommand()) // this mod already calls runplayermove on bots, we send the bot actual cmd on the hook
	{
		return;
	}

	if (sdkcalls->IsProcessUsercmdsAvailable())
	{
		sdkcalls->CBasePlayer_ProcessUsercmds(GetEntity(), ucmd);
		SnapEyeAngles(ucmd->viewangles);
	}
	else
	{
		m_controller->RunPlayerMove(ucmd);
	}
}

IPlayerController* CBaseBot::GetControlInterface() const
{
	if (!m_basecontrol)
	{
		m_basecontrol = std::make_unique<IPlayerController>(const_cast<CBaseBot*>(this));
	}

	return m_basecontrol.get();
}

IMovement* CBaseBot::GetMovementInterface() const
{
	if (!m_basemover)
	{
		m_basemover = std::make_unique<IMovement>(const_cast<CBaseBot*>(this));
	}

	return m_basemover.get();
}

ISensor* CBaseBot::GetSensorInterface() const
{
	if (!m_basesensor)
	{
		m_basesensor = std::make_unique<ISensor>(const_cast<CBaseBot*>(this));
	}

	return m_basesensor.get();
}

IBehavior* CBaseBot::GetBehaviorInterface() const
{
	if (!m_basebehavior)
	{
		m_basebehavior = std::make_unique<CBaseBotBehavior>(const_cast<CBaseBot*>(this));
	}

	return m_basebehavior.get();
}

IInventory* CBaseBot::GetInventoryInterface() const
{
	if (!m_baseinventory)
	{
		m_baseinventory = std::make_unique<IInventory>(const_cast<CBaseBot*>(this));
	}

	return m_baseinventory.get();
}

ISquad* CBaseBot::GetSquadInterface() const
{
	if (!m_basesquad)
	{
		m_basesquad = std::make_unique<ISquad>(const_cast<CBaseBot*>(this));
	}

	return m_basesquad.get();
}

void CBaseBot::Spawn()
{
	if (m_isfirstspawn == false)
	{
		m_isfirstspawn = true;
		FirstSpawn();
	}

#ifdef EXT_DEBUG

	auto speed = GetMaxSpeed();

	if (speed <= 0.1f)
	{
		// Warn developers if this is 0
		smutils->LogError(myself, "CBasePlayer::m_flMaxspeed == %4.8f", speed);
	}

#endif // EXT_DEBUG

	m_spawnTime = gpGlobals->curtime;
	m_homepos = GetAbsOrigin();
	m_justfiredmyweapon = false;
	Reset();
}

void CBaseBot::FirstSpawn()
{
}

void CBaseBot::Killed()
{
}

void CBaseBot::OnTakeDamage_Alive(const CTakeDamageInfo& info)
{
}

/**
 * @brief Selects weapon by using the IBotController::SetActiveWeapon function.
 * This will call BCC's Weapon_Create, make sure the bot actually owns the weapon you want or the bot will magically get one.
 * @param szclassname Weapon classname to select
*/
void CBaseBot::SelectWeaponByClassname(const char* szclassname)
{
	m_controller->SetActiveWeapon(szclassname);
}

/**
 * @brief Safe version of 'CBaseBot::SelectWeaponByClassname'. Adds an addition check to see if the bot actually owns the weapon
 * @param szclassname Weapon classname to select
*/
void CBaseBot::SafeWeaponSelectByClassname(const char* szclassname)
{
	if (Weapon_OwnsThisType(szclassname))
	{
		m_controller->SetActiveWeapon(szclassname);
	}
}

/**
 * @brief Makes the bot select a weapon via console command 'use'.
 * @param szclassname Classname of the weapon to select
*/
void CBaseBot::SelectWeaponByCommand(const char* szclassname) const
{
	char command[128];
	ke::SafeSprintf(command, sizeof(command), "use %s", szclassname);
	serverpluginhelpers->ClientCommand(GetEdict(), command);
}

void CBaseBot::FakeClientCommand(const char* command) const
{
	serverpluginhelpers->ClientCommand(GetEdict(), command);
}

void CBaseBot::ExecuteQueuedCommands()
{
	if (m_cmdqueue.empty())
	{
		return;
	}

	if (!m_cmdtimer.HasStarted() || m_cmdtimer.IsGreaterThen(1.1f))
	{
		m_cmdtimer.Start();
		m_cmdsents = 0; // reset the counter
	}

	if (m_cmdsents >= CBaseBot::m_maxStringCommandsPerSecond)
	{
		return;
	}

	while (m_cmdsents <= CBaseBot::m_maxStringCommandsPerSecond)
	{
		// Keep pushing commands until empty or we hit the limit per second

		auto& next = m_cmdqueue.front();
		serverpluginhelpers->ClientCommand(GetEdict(), next.c_str());
		m_cmdqueue.pop();
		m_cmdsents++;

		if (m_cmdqueue.empty())
		{
			return;
		}
	}

}

bool CBaseBot::IsLineOfFireClear(const Vector& to) const
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CBaseBot::IsLineOfFireClean", "NavBot");
#endif // EXT_VPROF_ENABLED

	CTraceFilterWorldAndPropsOnly filter;
	trace_t result;
	trace::line(GetEyeOrigin(), to, MASK_SHOT, &filter, result);
	return !result.DidHit();
}

int CBaseBot::GetWaterLevel() const
{
	return static_cast<int>(entityprops::GetEntityWaterLevel(GetEntity()));
}

bool CBaseBot::IsUnderWater() const
{
	return entityprops::GetEntityWaterLevel(GetEntity()) == static_cast<std::int8_t>(WaterLevel::WL_Eyes);
}

void CBaseBot::SendChatMessage(const char* message)
{
	constexpr std::size_t MAX_SIZE = 255U;

	std::unique_ptr<char[]> cmd = std::make_unique<char[]>(MAX_SIZE);

	ke::SafeSprintf(cmd.get(), MAX_SIZE, "say %s", message);
	DelayedFakeClientCommand(cmd.get());
}

void CBaseBot::SendTeamChatMessage(const char* message)
{
	constexpr std::size_t MAX_SIZE = 255U;

	std::unique_ptr<char[]> cmd = std::make_unique<char[]>(MAX_SIZE);

	ke::SafeSprintf(cmd.get(), MAX_SIZE, "say_team %s", message);
	DelayedFakeClientCommand(cmd.get());
}

void CBaseBot::FireWeaponAtEnemy(const CKnownEntity* enemy, const bool doAim)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CBaseBot::FireWeaponAtEnemy", "NavBot");
#endif // EXT_VPROF_ENABLED

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
}

bool CBaseBot::CanFireWeapon(const CBotWeapon* weapon, const float range, const bool allowSecondary, bool& doPrimary)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CBaseBot::CanFireWeapon", "NavBot");
#endif // EXT_VPROF_ENABLED

	auto& primaryinfo = weapon->GetWeaponInfo()->GetAttackInfo(WeaponInfo::PRIMARY_ATTACK);
	auto& secondaryinfo = weapon->GetWeaponInfo()->GetAttackInfo(WeaponInfo::SECONDARY_ATTACK);
	bool candoprimary = false;
	bool candosecondary = false;

	if (primaryinfo.HasFunction())
	{
		bool hasammo = true;

		if (!primaryinfo.IsMelee())
		{
			int ammo = 0;

			if (weapon->GetWeaponInfo()->Clip1IsReserveAmmo())
			{
				int index = weapon->GetBaseCombatWeapon().GetPrimaryAmmoType();

				if (index >= 0)
				{
					ammo = GetAmmoOfIndex(index);
				}
			}
			else
			{
				ammo = weapon->GetBaseCombatWeapon().GetClip1();
			}

			if (ammo <= 0)
			{
				hasammo = false;
			}
		}

		if (hasammo && (!primaryinfo.HasMinRange() || range >= primaryinfo.GetMinRange()) && (!primaryinfo.HasMaxRange() || range <= primaryinfo.GetMaxRange()))
		{
			candoprimary = true;
		}
	}

	if (secondaryinfo.HasFunction())
	{
		bool hasammo = true;

		if (!secondaryinfo.IsMelee())
		{
			int ammo = 0;

			if (weapon->GetWeaponInfo()->Clip2IsReserveAmmo())
			{
				int index = weapon->GetBaseCombatWeapon().GetSecondaryAmmoType();

				if (index >= 0)
				{
					ammo = GetAmmoOfIndex(index);
				}
			}
			else
			{
				ammo = weapon->GetBaseCombatWeapon().GetClip2();
			}

			if (ammo <= 0)
			{
				hasammo = false;
			}
		}

		if (hasammo && (!secondaryinfo.HasMinRange() || range >= secondaryinfo.GetMinRange()) && (!secondaryinfo.HasMaxRange() || range <= secondaryinfo.GetMaxRange()))
		{
			candosecondary = true;
		}
	}

	if (!candoprimary && !candosecondary)
	{
		return false;
	}

	if (candoprimary && !candosecondary)
	{
		doPrimary = true;
	}
	else if (!candoprimary && candosecondary)
	{
		doPrimary = false;
	}
	else
	{
		// can do both primary and seconadary
		// 20% to use secondary attack
		if (CBaseBot::s_botrng.GetRandomInt<int>(0, 5) == 5)
		{
			doPrimary = false;
		}
		else
		{
			doPrimary = true;
		}
	}

	return true;
}

bool CBaseBot::HandleWeapon(const CBotWeapon* weapon)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CBaseBot::HandleWeapon", "NavBot");
#endif // EXT_VPROF_ENABLED

	// this function is used for stuff like hold primary attack for N seconds then release to fire
	// or a weapon that must be deployed to be used
	// etc...

	if (HasJustFiredMyWeapon() && weapon->GetWeaponInfo()->IsSemiAuto())
	{
		SetJustFiredMyWeapon(false);
		GetControlInterface()->ReleaseAllAttackButtons();
		return false;
	}

	return true;
}

void CBaseBot::ReloadIfNeeded(const CBotWeapon* weapon)
{
	// Most mods only needs to reload the primary ammo

	if (m_reloadCheckDelay.IsElapsed())
	{
		auto& primaryinfo = weapon->GetWeaponInfo()->GetAttackInfo(WeaponInfo::PRIMARY_ATTACK);
		int primarytype = weapon->GetBaseCombatWeapon().GetPrimaryAmmoType();

		if (primarytype < 0)
			return;

		if (primaryinfo.HasFunction() && weapon->GetBaseCombatWeapon().GetClip1() == 0 && !primaryinfo.IsMelee())
		{
			if (GetAmmoOfIndex(primarytype) > 0 || weapon->GetWeaponInfo()->HasInfiniteAmmo())
			{
				GetControlInterface()->ReleaseAllAttackButtons();
				GetControlInterface()->PressReloadButton();
				m_reloadCheckDelay.Start(0.2f);
			}
		}
	}
}

bool CBaseBot::AimWeaponAtEnemy(const CKnownEntity* enemy, const CBotWeapon* weapon, const bool doAim, const float range, const bool isPrimary)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CBaseBot::AimWeaponAtEnemy", "NavBot");
#endif // EXT_VPROF_ENABLED

	IDecisionQuery::DesiredAimSpot desiredSpot = IDecisionQuery::DesiredAimSpot::AIMSPOT_NONE;

	auto& origin = enemy->GetEdict()->GetCollideable()->GetCollisionOrigin();
	auto center = UtilHelpers::getWorldSpaceCenter(enemy->GetEdict());
	auto& max = enemy->GetEdict()->GetCollideable()->OBBMaxs();
	Vector top = origin;
	top.z += max.z - 1.0f;

	bool headisclear = IsLineOfFireClear(top);
	bool canheadshot = false;
	const WeaponAttackFunctionInfo* funcinfo = nullptr;

	if (isPrimary)
	{
		funcinfo = &weapon->GetWeaponInfo()->GetAttackInfo(WeaponInfo::AttackFunctionType::PRIMARY_ATTACK);
	}
	else
	{
		funcinfo = &weapon->GetWeaponInfo()->GetAttackInfo(WeaponInfo::AttackFunctionType::SECONDARY_ATTACK);
	}

	// allowed to headshot when: the bot is allowed by the difficulty profile, the weapon is capable of headshots
	// and the enemy is within headshot range limits
	if (GetDifficultyProfile()->IsAllowedToHeadshot() && weapon->GetWeaponInfo()->CanHeadShot() &&
		range <= (funcinfo->GetMaxRange() * weapon->GetWeaponInfo()->GetHeadShotRangeMultiplier()))
	{
		canheadshot = true;
	}

	// priorize headshots if allowed
	if (canheadshot && headisclear)
	{
		// head is clear
		desiredSpot = IDecisionQuery::DesiredAimSpot::AIMSPOT_HEAD;
	}
	else if (IsLineOfFireClear(center))
	{
		// center is clear
		desiredSpot = IDecisionQuery::DesiredAimSpot::AIMSPOT_CENTER;
	}
	else if (IsLineOfFireClear(origin))
	{
		// abs origin is clear
		desiredSpot = IDecisionQuery::DesiredAimSpot::AIMSPOT_ABSORIGIN;
	}
	else if (GetDifficultyProfile()->IsAllowedToHeadshot() && headisclear)
	{
		// this handles cases where the weapon can't headshot but every body part minus the head is blocked
		desiredSpot = IDecisionQuery::DesiredAimSpot::AIMSPOT_HEAD;
	}
	else
	{
		return false; // all lines of fire are obstructed
	}

	if (doAim)
	{
		GetControlInterface()->SetDesiredAimSpot(desiredSpot);
		GetControlInterface()->AimAt(enemy->GetEntity(), IPlayerController::LOOK_COMBAT, 1.0f, "Aiming my weapon at the current visible threat!");
	}

	return true;
}
