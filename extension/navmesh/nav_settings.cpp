#include NAVBOT_PCH_FILE
#include "nav_mesh.h"
#include "nav_settings.h"

CBaseEntity* CNavMapSettings::EntityLookUpInfo::FindEntity() const
{
	if (HasHammerID())
	{
		return UtilHelpers::FindEntityByHammerID(hammerid);
	}

	if (classname.empty()) { return nullptr; }

	if (targetname.empty())
	{
		return UtilHelpers::GetNearestEntityOfClassname(position, classname.c_str());
	}

	return UtilHelpers::GetNearestNamedEntityOfClassname(position, classname.c_str(), targetname.c_str());
}

CNavMapSettings::CNavMapSettings()
{
	m_parser_section = 0;
	m_currentEntInfoPair = nullptr;
	m_additionaldoorclassnames.reserve(8);
	m_useableobstaclesinfo.reserve(8);
	m_useableobstaclesmap.reserve(8);
}

CNavMapSettings::~CNavMapSettings()
{
}

void CNavMapSettings::OnRoundRestart()
{
	LinkUseableEntities();
}

void CNavMapSettings::ReadSMC_ParseStart()
{
	m_parser_section = 0;
}

SourceMod::SMCResult CNavMapSettings::ReadSMC_NewSection(const SourceMod::SMCStates* states, const char* name)
{
	switch (m_parser_section)
	{
	case 0:
	{
		if (std::strcmp(name, "NavMapSettings") == 0)
		{
			m_parser_section = PARSER_SECTION_ROOT;
			break;
		}

		return SourceMod::SMCResult::SMCResult_HaltFail;
	}
	case PARSER_SECTION_ROOT:
	{
		if (ke::StrCaseCmp(name, "AdditionalDoorClassnames") == 0)
		{
			m_parser_section = PARSER_SECTION_ADDSDOORS;
			break;
		}

		if (ke::StrCaseCmp(name, "UseableObstacles") == 0)
		{
			m_parser_section = PARSER_SECTION_USEABLEOBSTACLES;
			break;
		}

		return SourceMod::SMCResult::SMCResult_HaltFail;
	}
	case PARSER_SECTION_USEABLEOBSTACLES:
	{
		if (ke::StrCaseCmp(name, "EntityInfoPair") == 0)
		{
			m_parser_section = PARSER_SECTION_UO_NEWENTINFOPAIR;
			auto& pair = m_useableobstaclesinfo.emplace_back();
			m_currentEntInfoPair = &pair;
			break;
		}
		return SourceMod::SMCResult::SMCResult_HaltFail;
	}
	case PARSER_SECTION_UO_NEWENTINFOPAIR:
	{
		if (ke::StrCaseCmp(name, "ObstacleEntityInfo") == 0)
		{
			m_parser_section = PARSER_SECTION_UO_OBSTACLEENTINFO;
			break;
		}

		if (ke::StrCaseCmp(name, "UseableEntityInfo") == 0)
		{
			m_parser_section = PARSER_SECTION_UO_USEABLEENTINFO;
			break;
		}

		return SourceMod::SMCResult::SMCResult_HaltFail;
	}
	default:
		return SourceMod::SMCResult::SMCResult_HaltFail;
	}

	return SourceMod::SMCResult::SMCResult_Continue;
}

SourceMod::SMCResult CNavMapSettings::ReadSMC_KeyValue(const SourceMod::SMCStates* states, const char* key, const char* value)
{
	switch (m_parser_section)
	{
	case PARSER_SECTION_ADDSDOORS:
	{
		ParseKeyValues_DoorSection(key);
		break;
	}
	case PARSER_SECTION_UO_OBSTACLEENTINFO:
		[[fallthrough]];
	case PARSER_SECTION_UO_USEABLEENTINFO:
		ParseKeyValues_UseableObstacles(key, value, m_parser_section == PARSER_SECTION_UO_USEABLEENTINFO);
		break;
	default:
		smutils->LogError(myself, "Unexpected key value pair (%s:%s) at line %u col %u!", key, value, states->line, states->col);
		return SourceMod::SMCResult::SMCResult_HaltFail;
	}

	return SourceMod::SMCResult::SMCResult_Continue;
}

SourceMod::SMCResult CNavMapSettings::ReadSMC_LeavingSection(const SourceMod::SMCStates* states)
{
	switch (m_parser_section)
	{
	case PARSER_SECTION_ADDSDOORS:
		m_parser_section = PARSER_SECTION_ROOT;
		break;
	case PARSER_SECTION_USEABLEOBSTACLES:
		m_parser_section = PARSER_SECTION_ROOT;
		break;
	case PARSER_SECTION_UO_NEWENTINFOPAIR:
		m_parser_section = PARSER_SECTION_USEABLEOBSTACLES;
		m_currentEntInfoPair = nullptr;
		break;
	case PARSER_SECTION_UO_OBSTACLEENTINFO:
		m_parser_section = PARSER_SECTION_UO_NEWENTINFOPAIR;
		break;
	case PARSER_SECTION_UO_USEABLEENTINFO:
		m_parser_section = PARSER_SECTION_UO_NEWENTINFOPAIR;
		break;
	case PARSER_SECTION_ROOT:
		m_parser_section = 0; 
		break;
	default:
		break;
	}

	return SourceMod::SMCResult::SMCResult_Continue;
}

void CNavMapSettings::PostParse()
{
}

bool CNavMapSettings::LoadFile(const std::filesystem::path& file)
{
	char error[1024];
	std::memset(error, 0, sizeof(error));
	SourceMod::SMCStates states;
	states.line = 0;
	states.col = 0;
	SourceMod::SMCError code = textparsers->ParseSMCFile(file.string().c_str(), this, &states, error, sizeof(error));

	if (code != SourceMod::SMCError::SMCError_Okay)
	{
		smutils->LogError(myself, "Error parsing nav mesh map settings file: %s", error);
		return false;
	}

	PostParse();

	return true;
}

void CNavMapSettings::ParseKeyValues_UseableObstacles(const char* key, const char* value, const bool isUseableEntity)
{
	// first is the obstacle entity
	// second is the useable entity
	EntityLookUpInfo* info = isUseableEntity ? &m_currentEntInfoPair->second : &m_currentEntInfoPair->first;

	if (ke::StrCaseCmp(key, "debugname") == 0)
	{
		info->debugname.assign(value);
	}
	else if (ke::StrCaseCmp(key, "classname") == 0)
	{
		info->classname.assign(value);
	}
	else if (ke::StrCaseCmp(key, "targetname") == 0)
	{
		info->targetname.assign(value);
	}
	else if (ke::StrCaseCmp(key, "position") == 0)
	{
		UtilHelpers::parsers::ParseVector(value, info->position);
	}
	else if (ke::StrCaseCmp(key, "hammerid") == 0)
	{
		info->hammerid = atoi(value);
	}
}

void CNavMapSettings::LinkUseableEntities()
{
	m_useableobstaclesmap.clear();

	for (auto& [obstacle, useable] : m_useableobstaclesinfo)
	{
		CBaseEntity* pObstacle = obstacle.FindEntity();
		CBaseEntity* pUseable = useable.FindEntity();

		if (pObstacle != nullptr && pUseable != nullptr && pObstacle != pUseable)
		{
			m_useableobstaclesmap.try_emplace(pObstacle, pUseable);
		}
		else
		{
			if (pObstacle == nullptr && !obstacle.debugname.empty())
			{
				smutils->LogError(myself, "Nav Mesh Useable Obstacles: Failed to link \"%s\" obstacle entity!", obstacle.debugname.c_str());
			}

			if (pUseable == nullptr && !useable.debugname.empty())
			{
				smutils->LogError(myself, "Nav Mesh Useable Obstacles: Failed to link \"%s\" useable entity!", useable.debugname.c_str());
			}

			if (pObstacle == pUseable && !obstacle.debugname.empty() && !useable.debugname.empty())
			{
				smutils->LogError(myself, "Nav Mesh Useable Obstacles: \"%s\" and \"%s\" linked to the same entity!", 
					obstacle.debugname.c_str(), useable.debugname.c_str());
			}
		}
	}
}
