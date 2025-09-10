#include NAVBOT_PCH_FILE
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <memory>
#include <filesystem>
#include <extension.h>
#include <manager.h>
#include <util/sdkcalls.h>
#include <bot/blackmesa/bmbot.h>
#include "nav/bm_nav_mesh.h"
#include "blackmesadm_mod.h"

class BMPlayerModelsFileParser : public ITextListener_INI
{
public:
	BMPlayerModelsFileParser()
	{
		playermodels.reserve(32);
		current = nullptr;
	}

	bool ReadINI_NewSection(const char* section, bool invalid_tokens, bool close_bracket, bool extra_tokens, unsigned int* curtok) override;
	bool ReadINI_KeyValue(const char* key, const char* value, bool invalid_tokens, bool equal_token, bool quotes, unsigned int* curtok) override;

	std::vector<std::pair<std::string, int>> playermodels;
	std::pair<std::string, int>* current;
};


CBlackMesaDeathmatchMod::CBlackMesaDeathmatchMod()
{
	m_isTeamPlay = false;
	std::fill(m_maxCarry.begin(), m_maxCarry.end(), 0);
	m_playermodels.reserve(32);

	ListenForGameEvent("round_start");
}

CBlackMesaDeathmatchMod::~CBlackMesaDeathmatchMod()
{
}

CBlackMesaDeathmatchMod* CBlackMesaDeathmatchMod::GetBMMod()
{
	return static_cast<CBlackMesaDeathmatchMod*>(extmanager->GetMod());
}

void CBlackMesaDeathmatchMod::FireGameEvent(IGameEvent* event)
{
	CBaseMod::FireGameEvent(event);

	if (event)
	{
		const char* name = event->GetName();

		if (std::strcmp(name, "round_start") == 0)
		{
			OnNewRound();
			OnRoundStart();
		}
	}
}

void CBlackMesaDeathmatchMod::PostCreation()
{
	CBaseMod::PostCreation();

	ParsePlayerModelsConfigFile();
}

CBaseBot* CBlackMesaDeathmatchMod::AllocateBot(edict_t* edict)
{
	return new CBlackMesaBot(edict);
}

CNavMesh* CBlackMesaDeathmatchMod::NavMeshFactory()
{
	return new CBlackMesaNavMesh;
}

void CBlackMesaDeathmatchMod::OnMapStart()
{
	CBaseMod::OnMapStart();

#if SOURCE_ENGINE == SE_BMS
	ConVarRef mp_teamplay("mp_teamplay");

	if (mp_teamplay.IsValid())
	{
		m_isTeamPlay = mp_teamplay.GetBool();
	}
	else
	{
		m_isTeamPlay = false;
		smutils->LogError(myself, "Failed to find ConVar \"mp_teamplay\"!");
	}
#endif // SOURCE_ENGINE == SE_BMS

#ifdef EXT_DEBUG
	rootconsole->ConsolePrint("Team Play is %s", m_isTeamPlay ? "ON" : "OFF");
#endif // EXT_DEBUG

	BuildMaxCarryArray();

	if (!sdkcalls->IsGetBoneTransformAvailable())
	{
		smutils->LogError(myself, "SDK Call to virtual CBaseAnimating::GetBoneTransform function unavailable! Check gamedata.");
		// this is required for Black Mesa since the bot aim relies on it.
		throw std::runtime_error("Critical gamedata failure for Black Mesa!");
	}
}

void CBlackMesaDeathmatchMod::OnRoundStart()
{
	librandom::ReSeedGlobalGenerators();
}

const std::pair<std::string, int>* CBlackMesaDeathmatchMod::GetRandomPlayerModel() const
{
	if (m_playermodels.empty())
	{
		return nullptr;
	}

	if (m_playermodels.size() == 1)
	{
		return &m_playermodels[0];
	}

	const std::pair<std::string, int>* result = &m_playermodels[CBaseBot::s_botrng.GetRandomInt<size_t>(0U, m_playermodels.size() - 1U)];
	return result;
}

void CBlackMesaDeathmatchMod::BuildMaxCarryArray()
{
#if SOURCE_ENGINE == SE_BMS
	ConVarRef sk_ammo_9mm_max("sk_ammo_9mm_max");

	if (sk_ammo_9mm_max.IsValid())
	{
		m_maxCarry[static_cast<int>(blackmesa::Ammo_9mm)] = sk_ammo_9mm_max.GetInt();
	}

	ConVarRef sk_ammo_357_max("sk_ammo_357_max");

	if (sk_ammo_357_max.IsValid())
	{
		m_maxCarry[static_cast<int>(blackmesa::Ammo_357)] = sk_ammo_357_max.GetInt();
	}

	ConVarRef sk_ammo_bolt_max("sk_ammo_bolt_max");

	if (sk_ammo_bolt_max.IsValid())
	{
		m_maxCarry[static_cast<int>(blackmesa::Ammo_Bolts)] = sk_ammo_bolt_max.GetInt();
	}

	ConVarRef sk_ammo_buckshot_max("sk_ammo_buckshot_max");

	if (sk_ammo_buckshot_max.IsValid())
	{
		m_maxCarry[static_cast<int>(blackmesa::Ammo_12ga)] = sk_ammo_buckshot_max.GetInt();
	}

	ConVarRef sk_ammo_energy_max("sk_ammo_energy_max");

	if (sk_ammo_energy_max.IsValid())
	{
		m_maxCarry[static_cast<int>(blackmesa::Ammo_DepletedUranium)] = sk_ammo_energy_max.GetInt();
	}

	ConVarRef sk_ammo_grenade_mp5_max("sk_ammo_grenade_mp5_max");

	if (sk_ammo_grenade_mp5_max.IsValid())
	{
		m_maxCarry[static_cast<int>(blackmesa::Ammo_40mmGL)] = sk_ammo_grenade_mp5_max.GetInt();
	}

	ConVarRef sk_ammo_grenade_rpg_max("sk_ammo_grenade_rpg_max");

	if (sk_ammo_grenade_rpg_max.IsValid())
	{
		m_maxCarry[static_cast<int>(blackmesa::Ammo_Rockets)] = sk_ammo_grenade_rpg_max.GetInt();
	}

	ConVarRef sk_ammo_grenade_frag_max("sk_ammo_grenade_frag_max");

	if (sk_ammo_grenade_frag_max.IsValid())
	{
		m_maxCarry[static_cast<int>(blackmesa::Ammo_FragGrenades)] = sk_ammo_grenade_frag_max.GetInt();
	}

	ConVarRef sk_ammo_grenade_satchel_max("sk_ammo_grenade_satchel_max");

	if (sk_ammo_grenade_satchel_max.IsValid())
	{
		m_maxCarry[static_cast<int>(blackmesa::Ammo_Satchel)] = sk_ammo_grenade_satchel_max.GetInt();
	}

	ConVarRef sk_ammo_grenade_tripmine_max("sk_ammo_grenade_tripmine_max");

	if (sk_ammo_grenade_tripmine_max.IsValid())
	{
		m_maxCarry[static_cast<int>(blackmesa::Ammo_Tripmines)] = sk_ammo_grenade_tripmine_max.GetInt();
	}

	ConVarRef sk_ammo_hornet_max("sk_ammo_hornet_max");

	if (sk_ammo_hornet_max.IsValid())
	{
		m_maxCarry[static_cast<int>(blackmesa::Ammo_Hornets)] = sk_ammo_hornet_max.GetInt();
	}

	ConVarRef sk_ammo_snark_max("sk_ammo_snark_max");

	if (sk_ammo_snark_max.IsValid())
	{
		m_maxCarry[static_cast<int>(blackmesa::Ammo_Snarks)] = sk_ammo_snark_max.GetInt();
	}
#endif // SOURCE_ENGINE == SE_BMS
}

void CBlackMesaDeathmatchMod::ParsePlayerModelsConfigFile()
{
	std::unique_ptr<char[]> path = std::make_unique<char[]>(PLATFORM_MAX_PATH);
	const char* modfolder = smutils->GetGameFolderName();

	smutils->BuildPath(SourceMod::Path_SM, path.get(), PLATFORM_MAX_PATH, "configs/navbot/%s/player_models.custom.ini", modfolder);
	
	if (!std::filesystem::exists(path.get()))
	{
		smutils->BuildPath(SourceMod::Path_SM, path.get(), PLATFORM_MAX_PATH, "configs/navbot/%s/player_models.ini", modfolder);
	}

	if (!std::filesystem::exists(path.get()))
	{
		smutils->LogError(myself, "Failed to parse Black Mesa player models configuration file. File \"%s\" does not exists!", path.get());
		return;
	}

	BMPlayerModelsFileParser parser;
	unsigned int line = 0;
	unsigned int col = 0;
	bool result = textparsers->ParseFile_INI(path.get(), &parser, &line, &col);

	if (!result)
	{
		smutils->LogError(myself, "Failed to parse Black Mesa player models configuration file. Parser failed to for \"%s\"! Line %i col %i", path.get(), line, col);
		return;
	}

	if (parser.playermodels.empty())
	{
		return;
	}

	m_playermodels.swap(parser.playermodels);
}

bool BMPlayerModelsFileParser::ReadINI_NewSection(const char* section, bool invalid_tokens, bool close_bracket, bool extra_tokens, unsigned int* curtok)
{
	if (!section || strcmp(section, "") == 0 || strlen(section) < 2)
	{
		return true;
	}

	current = &playermodels.emplace_back(section, 0);

	return true;
}

bool BMPlayerModelsFileParser::ReadINI_KeyValue(const char* key, const char* value, bool invalid_tokens, bool equal_token, bool quotes, unsigned int* curtok)
{
	if (!equal_token)
	{
		return true;
	}

	if (key && strncasecmp(key, "skin_count", 10) == 0)
	{
		int quantity = atoi(value);
		current->second = quantity;
	}

	return true;
}