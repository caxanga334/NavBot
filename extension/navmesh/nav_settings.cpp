#include NAVBOT_PCH_FILE
#include "nav_mesh.h"
#include "nav_settings.h"

CNavMapSettings::CNavMapSettings()
{
	m_parser_section = 0;
	m_additionaldoorclassnames.reserve(8);
}

CNavMapSettings::~CNavMapSettings()
{
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
	case PARSER_SECTION_ROOT:
		m_parser_section = 0;
		break;
	default:
		break;
	}

	return SourceMod::SMCResult::SMCResult_Continue;
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

	return true;
}
