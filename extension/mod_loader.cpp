#include NAVBOT_PCH_FILE
#include <mods/basemod.h>
#include <mods/tf2/teamfortress2mod.h>
#include <mods/blackmesa/blackmesadm_mod.h>
#include <mods/dods/dayofdefeatsourcemod.h>
#include <mods/hl2dm/hl2dm_mod.h>
#include <mods/zps/zps_mod.h>
#include "mod_loader.h"

ExtModLoader::ExtModLoader()
{
	m_detectedMod = nullptr;
	m_parser_current = nullptr;
	m_parser_section = PARSER_SECTION_ROOT;
}

ExtModLoader::~ExtModLoader()
{
}

bool ExtModLoader::ModLoaderFileExists()
{
	char path[PLATFORM_MAX_PATH];
	smutils->BuildPath(SourceMod::PathType::Path_SM, path, sizeof(path), "configs/navbot/mod_loader.custom.cfg");

	bool found = std::filesystem::exists(path);

	if (!found)
	{
		smutils->BuildPath(SourceMod::PathType::Path_SM, path, sizeof(path), "configs/navbot/mod_loader.cfg");
		found = std::filesystem::exists(path);
	}

	return found;
}

void ExtModLoader::DetectMod()
{
	char path[PLATFORM_MAX_PATH];
	smutils->BuildPath(SourceMod::PathType::Path_SM, path, sizeof(path), "configs/navbot/mod_loader.custom.cfg");

	if (!std::filesystem::exists(path))
	{
		smutils->BuildPath(SourceMod::PathType::Path_SM, path, sizeof(path), "configs/navbot/mod_loader.cfg");
	}

	// this should be impossible since the extension should have failed to load if this file is missing.
	if (!std::filesystem::exists(path))
	{
		throw std::runtime_error{ "NavBot: Missing mod loader config file!" };
	}

	char error[512];
	SourceMod::SMCStates states;
	SourceMod::SMCError code = textparsers->ParseSMCFile(path, this, &states, error, sizeof(error));

	if (code != SourceMod::SMCError::SMCError_Okay)
	{
		smutils->LogError(myself, "Failed to parse mod loader config! Error: %s (%s)", error, textparsers->GetSMCErrorString(code));
		// the mod loader is critical, throw an exception here to crash
		throw std::runtime_error{ "NavBot: Mod loader config parse failure!" };
	}

	const ModEntry* detected = nullptr;
	const char* gamefolder = smutils->GetGameFolderName();
	std::uint64_t appid = DetectAppID();

	if (appid == 0)
	{
		smutils->LogMessage(myself, "[Mod Loader] Warning! Failed to detect app ID!");
	}

	for (const ModEntry& entry : m_mods)
	{
		switch (entry.detection_type)
		{
		case ModDetectionType::DETECTION_MOD_FOLDER:
		{
			if (std::strcmp(gamefolder, entry.detection_data.c_str()) == 0)
			{
				detected = &entry;
			}

			break;
		}
		case ModDetectionType::DETECTION_NETPROPERTY:
		{
			if (!entry.detection_sendprop_class.empty() && !entry.detection_sendprop_property.empty())
			{
				SourceMod::sm_sendprop_info_t info;

				if (gamehelpers->FindSendPropInfo(entry.detection_sendprop_class.c_str(), entry.detection_sendprop_property.c_str(), &info))
				{
					detected = &entry;
				}
			}

			break;
		}
		case ModDetectionType::DETECTION_SERVERCLASS:
		{
			if (!entry.detection_sendprop_class.empty())
			{
				if (gamehelpers->FindServerClass(entry.detection_sendprop_class.c_str()) != nullptr)
				{
					detected = &entry;
				}
			}

			break;
		}
		case ModDetectionType::DETECTION_APPID:
		{
			if (entry.detection_appid != 0 && appid != 0)
			{
				if (entry.detection_appid == appid)
				{
					detected = &entry;
				}
			}

			break;
		}
		}

		if (detected)
		{
			break;
		}
	}

	if (!detected)
	{
		smutils->LogError(myself, "[Mod Loader] Failed to detect current game/mod!");
	}
	else
	{
		smutils->LogMessage(myself, "[Mod Loader] Detected %s", detected->name.c_str());
	}

	m_detectedMod = detected;
}

CBaseMod* ExtModLoader::AllocDetectedMod() const
{
	CBaseMod* mod = nullptr;

	if (!m_detectedMod)
	{
		// detection failed;
		mod = new CBaseMod;
		mod->m_modID = Mods::ModType::MOD_BASE;
		mod->m_modName.assign("CBaseMod");
		return mod;
	}

	switch (m_detectedMod->id)
	{
	case Mods::ModType::MOD_TF2:
		mod = new CTeamFortress2Mod;
		break;
	case Mods::ModType::MOD_DODS:
		mod = new CDayOfDefeatSourceMod;
		break;
	case Mods::ModType::MOD_BLACKMESA:
		mod = new CBlackMesaDeathmatchMod;
		break;
	case Mods::ModType::MOD_HL2DM:
		mod = new CHalfLife2DeathMatchMod;
		break;
	case Mods::ModType::MOD_ZPS:
		mod = new CZombiePanicSourceMod;
		break;
	default:
		mod = new CBaseMod;
		break;
	}

	mod->m_modID = m_detectedMod->id;
	mod->m_modName = m_detectedMod->name;

	// override mod folder for config files if needed
	if (!m_detectedMod->mod_folder.empty())
	{
		mod->m_modFolder = m_detectedMod->mod_folder;
	}

	return mod;
}

void ExtModLoader::ReadSMC_ParseStart()
{
	m_mods.clear();
	m_parser_current = nullptr;
	m_parser_section = PARSER_SECTION_ROOT;
	m_detectedMod = nullptr;
}

SourceMod::SMCResult ExtModLoader::ReadSMC_NewSection(const SourceMod::SMCStates* states, const char* name)
{
	switch (m_parser_section)
	{
	case PARSER_SECTION_ROOT:
	{
		if (std::strcmp(name, "ModLoader") == 0)
		{
			m_parser_section = PARSER_SECTION_BASE;
			return SourceMod::SMCResult::SMCResult_Continue;
		}

		return SourceMod::SMCResult::SMCResult_HaltFail;
	}
	case PARSER_SECTION_BASE:
	{
		m_parser_current = &m_mods.emplace_back();

		m_parser_current->entry.assign(name);
		m_parser_section = PARSER_SECTION_MODENTRY;
		return SourceMod::SMCResult::SMCResult_Continue;
	}
	case PARSER_SECTION_MODENTRY:
	{
		if (std::strcmp(name, "Description") == 0)
		{
			m_parser_section = PARSER_SECTION_MODDESC;
			return SourceMod::SMCResult::SMCResult_Continue;
		}

		if (std::strcmp(name, "Config") == 0)
		{
			m_parser_section = PARSER_SECTION_MODCONF;
			return SourceMod::SMCResult::SMCResult_Continue;
		}

		if (std::strcmp(name, "Detection") == 0)
		{
			m_parser_section = PARSER_SECTION_MODDECT;
			return SourceMod::SMCResult::SMCResult_Continue;
		}

		break;
	}
	default:
		break;
	}

	return SourceMod::SMCResult::SMCResult_HaltFail;
}

SourceMod::SMCResult ExtModLoader::ReadSMC_KeyValue(const SourceMod::SMCStates* states, const char* key, const char* value)
{
	switch (m_parser_section)
	{
	case PARSER_SECTION_MODDECT:
	{
		if (std::strcmp(key, "Type") == 0)
		{
			if (std::strcmp(value, "GameFolder") == 0)
			{
				m_parser_current->detection_type = ModDetectionType::DETECTION_MOD_FOLDER;
			}
			else if (std::strcmp(value, "NetProp") == 0)
			{
				m_parser_current->detection_type = ModDetectionType::DETECTION_NETPROPERTY;
			}
			else if (std::strcmp(value, "ServerClass") == 0)
			{
				m_parser_current->detection_type = ModDetectionType::DETECTION_SERVERCLASS;
			}
			else if (std::strcmp(value, "AppID") == 0)
			{
				m_parser_current->detection_type = ModDetectionType::DETECTION_APPID;
			}
			else
			{
				smutils->LogError(myself, "Unknown mod detection type %s at line %u col %u!", value, states->line, states->col);
				return SourceMod::SMCResult::SMCResult_HaltFail;
			}
		}
		else if (std::strcmp(key, "Data") == 0)
		{
			m_parser_current->detection_data.assign(value);
		}
		else if (std::strcmp(key, "PropClass") == 0)
		{
			m_parser_current->detection_sendprop_class.assign(value);
		}
		else if (std::strcmp(key, "PropName") == 0)
		{
			m_parser_current->detection_sendprop_property.assign(value);
		}
		else if (std::strcmp(key, "AppID") == 0)
		{
			std::string szValue{ value };
			m_parser_current->detection_appid = static_cast<std::uint64_t>(std::stoull(szValue));
		}
		break;
	}
	case PARSER_SECTION_MODENTRY:
	{
		if (std::strcmp(key, "inherit_from") == 0)
		{
			for (auto& entries : m_mods)
			{
				if (&entries == m_parser_current)
				{
					continue;
				}

				if (std::strcmp(value, entries.entry.c_str()) == 0)
				{
					CopyModEntry(&entries, m_parser_current);
					return SourceMod::SMCResult::SMCResult_Continue;
				}
			}
		}

		break;
	}
	case PARSER_SECTION_MODDESC:
	{
		if (std::strcmp(key, "Name") == 0)
		{
			m_parser_current->name.assign(value);
		}
		else if (std::strcmp(key, "ID") == 0)
		{
			Mods::ModType id = static_cast<Mods::ModType>(atoi(value));

			if (id < Mods::ModType::MOD_BASE)
			{
				id = Mods::ModType::MOD_BASE;
			}

			m_parser_current->id = id;
		}

		break;
	}
	case PARSER_SECTION_MODCONF:
	{
		if (std::strcmp(key, "Mod_Folder") == 0)
		{
			m_parser_current->mod_folder.assign(value);
		}

		break;
	}
	default:
		break;
	}

	return SourceMod::SMCResult::SMCResult_Continue;
}

SourceMod::SMCResult ExtModLoader::ReadSMC_LeavingSection(const SourceMod::SMCStates* states)
{
	switch (m_parser_section)
	{
	case PARSER_SECTION_BASE:
	{
		m_parser_section = PARSER_SECTION_ROOT;
		break;
	}
	case PARSER_SECTION_MODENTRY:
	{
		m_parser_current = nullptr;
		m_parser_section = PARSER_SECTION_BASE;
		break;
	}
	case PARSER_SECTION_MODCONF:
		[[fallthrough]];
	case PARSER_SECTION_MODDESC:
	{
		m_parser_section = PARSER_SECTION_MODENTRY;
		break;
	}
	case PARSER_SECTION_MODDECT:
	{
		if (m_parser_current->detection_type == ModDetectionType::DETECTION_DEFAULT)
		{
			smutils->LogError(myself, "Mod loader entry \"%s\" with default detection type!", m_parser_current->entry.c_str());
			return SourceMod::SMCResult::SMCResult_HaltFail;
		}

		if (m_parser_current->detection_type == ModDetectionType::DETECTION_MOD_FOLDER && m_parser_current->detection_data.empty())
		{
			smutils->LogError(myself, "Mod loader entry \"%s\" missing detection data!", m_parser_current->entry.c_str());
			return SourceMod::SMCResult::SMCResult_HaltFail;
		}

		if (m_parser_current->detection_type == ModDetectionType::DETECTION_SERVERCLASS && m_parser_current->detection_sendprop_class.empty())
		{
			smutils->LogError(myself, "Mod loader entry \"%s\" missing detection data!", m_parser_current->entry.c_str());
			return SourceMod::SMCResult::SMCResult_HaltFail;
		}

		if (m_parser_current->detection_type == ModDetectionType::DETECTION_NETPROPERTY &&
			(m_parser_current->detection_sendprop_property.empty() || m_parser_current->detection_sendprop_class.empty()))
		{
			smutils->LogError(myself, "Mod loader entry \"%s\" missing detection data!", m_parser_current->entry.c_str());
			return SourceMod::SMCResult::SMCResult_HaltFail;
		}

		m_parser_section = PARSER_SECTION_MODENTRY;
		break;
	}
	default:
		break;
	}

	return SourceMod::SMCResult::SMCResult_Continue;
}

std::uint64_t ExtModLoader::DetectAppID()
{
	char path[PLATFORM_MAX_PATH];
	smutils->BuildPath(SourceMod::PathType::Path_Game, path, sizeof(path), "steam.inf");

	if (!std::filesystem::exists(path))
	{
		return 0;
	}

	std::fstream file;

	file.open(path, std::ios_base::in);

	if (!file.is_open())
	{
		return 0;
	}

	constexpr std::string_view strAppID{ "appID=" };

	std::string line;
	while (std::getline(file, line))
	{
		std::size_t pos = line.find(strAppID.data(), 0);

		if (pos == std::string::npos)
		{
			continue;
		}

		std::string szAppID = line.substr(strAppID.size());

		return static_cast<std::uint64_t>(std::stoull(szAppID));
	}

	return 0;
}
