#include NAVBOT_PCH_FILE
#include <bot/css/cssbot.h>
#include "nav/css_nav_mesh.h"
#include "css_mod.h"

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

SourceMod::SMCResult CCSSModSettings::ReadSMC_KeyValue(const SourceMod::SMCStates* states, const char* key, const char* value)
{
	if (std::strcmp(key, "eco_limit") == 0)
	{
		SetEcoLimit(std::clamp(atoi(value), 1000, 4000));
		return SourceMod::SMCResult::SMCResult_Continue;
	}

	if (std::strcmp(key, "team_eco_limit") == 0)
	{
		SetTeamEcoLimit(std::clamp(atoi(value), 500, 4500));
		return SourceMod::SMCResult::SMCResult_Continue;
	}

	if (std::strcmp(key, "timeleft_to_hurry") == 0)
	{
		SetTimeLeftToHurry(std::clamp(static_cast<float>(atof(value)), 0.0f, 60.0f));
		return SourceMod::SMCResult::SMCResult_Continue;
	}

	return CModSettings::ReadSMC_KeyValue(states, key, value);
}

CCounterStrikeSourceMod::CCounterStrikeSourceMod()
{
	ListenForGameEvent("round_start");
	ListenForGameEvent("round_end");
	ListenForGameEvent("flashbang_detonate");
	ListenForGameEvent("bomb_planted");
	ListenForGameEvent("hostage_follows");

	std::fill(m_teammoney.begin(), m_teammoney.end(), 0);
	m_bombactive = false;
	m_bombisknown = false;
	m_hostagestaken = false;
	m_c4.Term();
}

void CCounterStrikeSourceMod::FireGameEvent(IGameEvent* event)
{
	if (event)
	{
		const char* name = event->GetName();

#ifdef EXT_DEBUG
		META_CONPRINTF("CCounterStrikeSourceMod::FireGameEvent: \"%s\" \n", name);
#endif // EXT_DEBUG

		if (std::strcmp(name, "round_start") == 0)
		{
			OnRoundStart();
		}
		else if (std::strcmp(name, "round_end") == 0)
		{
			OnRoundEnd();
		}
		else if (std::strcmp(name, "flashbang_detonate") == 0)
		{
			Vector pos{ event->GetFloat("x"), event->GetFloat("y"), event->GetFloat("z") };
			OnFlashbangDetonated(event->GetInt("userid", INVALID_EHANDLE_INDEX), pos);
		}
		else if (std::strcmp(name, "bomb_planted") == 0)
		{
			OnBombPlanted(event->GetInt("userid"));
		}
		else if (std::strcmp(name, "hostage_follows") == 0)
		{
			m_hostagestaken = true;
		}
	}

	CBaseMod::FireGameEvent(event);
}

CBaseBot* CCounterStrikeSourceMod::AllocateBot(edict_t* edict)
{
	return new CCSSBot(edict);
}

void CCounterStrikeSourceMod::OnRoundStart()
{
	CBaseMod::OnRoundStart();
	RoundReset();
	librandom::ReSeedGlobalGenerators();

	std::array<int, static_cast<std::size_t>(counterstrikesource::CSSTeam::MAX_CSS_TEAMS)> totalmoney;
	std::array<int, static_cast<std::size_t>(counterstrikesource::CSSTeam::MAX_CSS_TEAMS)> numplayers;
	std::fill(totalmoney.begin(), totalmoney.end(), 0);
	std::fill(numplayers.begin(), numplayers.end(), 0);
	std::fill(m_teammoney.begin(), m_teammoney.end(), 0);

	auto func = [&totalmoney, &numplayers](int client, edict_t* entity, SourceMod::IGamePlayer* player) {
		if (player->IsInGame() && player->GetPlayerInfo() != nullptr)
		{
			counterstrikesource::CSSTeam team = static_cast<counterstrikesource::CSSTeam>(player->GetPlayerInfo()->GetTeamIndex());
			CBaseEntity* pEntity = UtilHelpers::EdictToBaseEntity(entity);
			
			switch (team)
			{
			case counterstrikesource::CSSTeam::TERRORIST:
				[[fallthrough]];
			case counterstrikesource::CSSTeam::COUNTERTERRORIST:
			{
				totalmoney[static_cast<int>(team)] += csslib::GetPlayerMoney(pEntity);
				numplayers[static_cast<int>(team)] += 1;

				break;
			}
			default:
				break;
			}
		}
	};

	UtilHelpers::ForEachPlayer(func);

	constexpr int tridx = static_cast<int>(counterstrikesource::CSSTeam::TERRORIST);
	constexpr int ctidx = static_cast<int>(counterstrikesource::CSSTeam::COUNTERTERRORIST);

	if (numplayers[tridx] > 0)
	{
		m_teammoney[tridx] = totalmoney[tridx] / numplayers[tridx];
	}

	if (numplayers[ctidx] > 0)
	{
		m_teammoney[ctidx] = totalmoney[ctidx] / numplayers[ctidx];
	}
}

void CCounterStrikeSourceMod::OnRoundEnd()
{
	CBaseMod::OnRoundEnd();
	RoundReset();
}

CNavMesh* CCounterStrikeSourceMod::NavMeshFactory()
{
	return new CCSSNavMesh;
}

static bool TryBuyingItem(CCSSBot* bot, const counterstrikesource::BuyManager& mgr, int& money, const char* item)
{
	if (!mgr.IsAvailableToTeam(item, bot->GetMyCSSTeam()))
	{
		return false;
	}

	int price = mgr.LookupPrice(item);

	if (price <= money)
	{
		bot->SendBuyCommand(item);
		money -= price;

		if (bot->IsDebugging(BOTDEBUG_MISC))
		{
			bot->DebugPrintToConsole(255, 215, 0, "%s IS BUYING \"%s\"! MONEY LEFT: %i \n", bot->GetDebugIdentifier(), item, money);
		}

		return true;
	}

	return false;
}

void CCounterStrikeSourceMod::BuyWeapons(CCSSBot* bot) const
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CCounterStrikeSourceMod::BuyWeapons", "NavBot");
#endif // EXT_VPROF_ENABLED

	using namespace std::literals::string_view_literals;
	const counterstrikesource::BuyProfile* profile = bot->GetBuyProfile();
	const bool debugging = bot->IsDebugging(BOTDEBUG_MISC);

	if (!profile)
	{
		if (debugging)
		{
			bot->DebugPrintToConsole(255, 0, 0, "%s CANNOT BUY WEAPONS! NULL BUY PROFILE! \n", bot->GetDebugIdentifier());
		}

		return;
	}

	if (!csslib::IsInBuyZone(bot->GetEntity()))
	{
		if (debugging)
		{
			Vector pos = bot->GetAbsOrigin();
			bot->DebugPrintToConsole(255, 0, 0, "%s CANNOT BUY WEAPONS! NOT IN A BUY ZONE AT <%s> \n", 
				bot->GetDebugIdentifier(), UtilHelpers::textformat::FormatVector(pos));
		}

		return;
	}

	if (debugging)
	{
		bot->DebugPrintToConsole(0, 128, 0, "%s RUNNING BUY LOGIC! PROFILE: %s \n", bot->GetDebugIdentifier(), profile->m_name.c_str());
	}

	auto myteam = bot->GetMyCSSTeam();

	if (m_teammoney[static_cast<int>(myteam)] < GetCSSModSettings()->GetTeamEcoLimit())
	{
		if (debugging)
		{
			bot->DebugPrintToConsole(255, 0, 0, "%s CANNOT BUY WEAPONS! TEAM IS LOW ON MONEY! %i < %i \n",
				bot->GetDebugIdentifier(), m_teammoney[static_cast<int>(myteam)], GetCSSModSettings()->GetTeamEcoLimit());
		}

		return;
	}

	int money = csslib::GetPlayerMoney(bot->GetEntity());

	if (money < GetCSSModSettings()->GetEcoLimit())
	{
		if (debugging)
		{
			bot->DebugPrintToConsole(255, 0, 0, "%s CANNOT BUY WEAPONS! CURRENT MONEY %i IS LESS THAN ECO LIMIT %i! \n",
				bot->GetDebugIdentifier(), money, GetCSSModSettings()->GetEcoLimit());
		}

		return;
	}

	auto& mgr = GetBuyManager();

	constexpr auto armoritemname = "vesthelm"sv;

	// armor
	if (bot->GetDifficultyProfile()->GetGameAwareness() >= 40)
	{
		if (bot->GetArmorAmount() < 100 || !csslib::HasHelmetEquipped(bot->GetEntity()))
		{
			TryBuyingItem(bot, mgr, money, armoritemname.data());
		}
	}

	// try buying a primary weapon

	auto inventory = bot->GetInventoryInterface();

	if (inventory->FindWeaponBySlot(counterstrikesource::CSS_WEAPON_SLOT_PRIMARY) == nullptr)
	{
		for (auto& weapon : profile->m_primaries)
		{
			if (!weapon.empty())
			{
				if (TryBuyingItem(bot, mgr, money, weapon.c_str()))
				{
					break;
				}
			}
		}
	}

	constexpr auto defuseritemname = "defuser"sv;

	if (myteam == counterstrikesource::CSSTeam::COUNTERTERRORIST &&
		!csslib::HasDefuser(bot->GetEntity()) &&
		CBaseBot::s_botrng.GetRandomChance(profile->m_defuserchance))
	{
		TryBuyingItem(bot, mgr, money, defuseritemname.data());
	}

	if (money >= profile->m_minmoneyfornades)
	{
		constexpr auto heitemname = "hegrenade"sv;
		constexpr auto flashitemname = "flashbang"sv;
		constexpr auto smokeitemname = "smokegrenade"sv;

		if (inventory->FindWeaponByClassname("weapon_hegrenade") == nullptr && CBaseBot::s_botrng.GetRandomChance(profile->m_hechance))
		{
			TryBuyingItem(bot, mgr, money, heitemname.data());
		}

		if (inventory->FindWeaponByClassname("weapon_flashbang") == nullptr && CBaseBot::s_botrng.GetRandomChance(profile->m_flashchance))
		{
			TryBuyingItem(bot, mgr, money, flashitemname.data());
		}

		if (inventory->FindWeaponByClassname("weapon_smokegrenade") == nullptr && CBaseBot::s_botrng.GetRandomChance(profile->m_smokechance))
		{
			TryBuyingItem(bot, mgr, money, smokeitemname.data());
		}
	}

	if (money >= profile->m_minmoneyforsecupgrade && inventory->HasStartingPistol())
	{
		for (auto& weapon : profile->m_secondaries)
		{
			if (!weapon.empty())
			{
				if (TryBuyingItem(bot, mgr, money, weapon.c_str()))
				{
					break;
				}
			}
		}
	}
}

CCounterStrikeSourceMod* CCounterStrikeSourceMod::GetCSSMod()
{
	return static_cast<CCounterStrikeSourceMod*>(extmanager->GetMod());
}

void CCounterStrikeSourceMod::RegisterModCommands()
{
	auto& mgr = extmanager->GetServerCommandManager();
	mgr.RegisterConCommand("sm_navbot_css_reload_buy_manager", "Reloads the buy manager configuration file.", FCVAR_GAMEDLL, CCounterStrikeSourceMod::OnReloadBuyManagerCommand);
}

void CCounterStrikeSourceMod::OnPostInit()
{
	if (!m_buymanager.ParseConfigFile())
	{
		smutils->LogError(myself, "Error while parsing buy manager config file!");
	}
}

void CCounterStrikeSourceMod::OnFlashbangDetonated(int userid, const Vector& pos)
{
#ifdef EXT_DEBUG
	META_CONPRINTF("%3.2f: [NavBot] OnFlashbangDetonated %i <%s> \n", gpGlobals->curtime, userid, UtilHelpers::textformat::FormatVector(pos));
	
	auto dbgfunc = [](int client, edict_t* entity, SourceMod::IGamePlayer* player) {
		if (player->IsInGame())
		{
			CBaseEntity* pEntity = UtilHelpers::EdictToBaseEntity(entity);

			if (pEntity)
			{
				float flashtime = csslib::GetFlashbangMaxDuration(pEntity);
				float flashalpha = csslib::GetFlashbangMaxAlpha(pEntity);

				META_CONPRINTF("%s [%i:%i] flash length: %3.2f flash alpha: %3.2f\n", player->GetName(), client, player->GetUserId(), flashtime, flashalpha);
			}
		}
	};

	UtilHelpers::ForEachPlayer(dbgfunc);

#endif // EXT_DEBUG

	auto func = [&pos](CBaseBot* bot) {
		CCSSBot* csbot = static_cast<CCSSBot*>(bot);

		float flashtime = csslib::GetFlashbangMaxDuration(csbot->GetEntity());

		// not flashed
		if (flashtime <= 0.05f) { return; }

		float flashalpha = csslib::GetFlashbangMaxAlpha(csbot->GetEntity());

		// if not fully flashed, subtract 0.5 seconds from the flash time
		if (flashalpha < 250.0f)
		{
			flashtime -= 0.5f;
			flashtime = std::max(flashtime, 0.25f);
		}

		csbot->GetCombatInterface()->SetFlashbangedTime(flashtime);

		if (csbot->IsDebugging(BOTDEBUG_COMBAT))
		{
			csbot->DebugPrintToConsole(255, 255, 0, "%s IS FLASHED FOR %3.2f SECONDS (ALPHA: %3.2f) \n", csbot->GetDebugIdentifier(), flashtime, flashalpha);
		}
	};

	extmanager->ForEachBot(func);
}

void CCounterStrikeSourceMod::OnBombPlanted(int userid)
{
	m_bombactive = true;
	int bomb = UtilHelpers::FindEntityByClassname(INVALID_EHANDLE_INDEX, "planted_c4");
	Vector pos{ 0.0f, 0.0f, 0.0f };
	CBaseEntity* pBomb = nullptr;
	CBaseEntity* pPlayer = nullptr;
	int client = playerhelpers->GetClientOfUserId(userid);

	if (client != 0)
	{
		pPlayer = gamehelpers->ReferenceToEntity(client);
	}

	if (bomb != INVALID_EHANDLE_INDEX)
	{
		pBomb = gamehelpers->ReferenceToEntity(bomb);
		pos = UtilHelpers::getEntityOrigin(pBomb);
		m_c4 = pBomb;
	}

	auto func = [pBomb, pos, pPlayer](CBaseBot* bot) {
		bot->OnBombPlanted(pos, static_cast<int>(counterstrikesource::CSSTeam::TERRORIST), pPlayer, pBomb);
	};

	extmanager->ForEachBot(func);

#ifdef EXT_DEBUG
	META_CONPRINTF("[NavBot] OnBombPlanted: bomb entity %s <%s>\n", UtilHelpers::textformat::FormatEntity(pBomb), UtilHelpers::textformat::FormatVector(pos));
#endif // EXT_DEBUG
}

void CCounterStrikeSourceMod::RoundReset()
{
	m_bombactive = false;
	m_bombisknown = false;
	m_hostagestaken = false;
	m_c4.Term();
}

void CCounterStrikeSourceMod::OnReloadBuyManagerCommand(const CConCommandArgs& args)
{
	if (CCounterStrikeSourceMod::GetCSSMod()->m_buymanager.ParseConfigFile())
	{
		META_CONPRINT("Buy manager config file reloaded without errors.\n");
	}
	else
	{
		META_CONPRINT("Error while reloading the buy manager config, check your logs!\n");
	}

	auto func = [](CBaseBot* bot) {
		static_cast<CCSSBot*>(bot)->SetBuyProfile(CCounterStrikeSourceMod::GetCSSMod()->GetBuyManager().GetRandomBuyProfile());
	};

	extmanager->ForEachBot(func);
}
