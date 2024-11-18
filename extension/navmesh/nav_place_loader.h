#ifndef NAV_PLACE_LOADER_H_
#define NAV_PLACE_LOADER_H_

#include <string>
#include <unordered_map>
#include <filesystem>
#include <cstring>
#include <smsdk_ext.h>
#include "nav.h"

class NavPlaceDatabaseLoader : public SourceMod::ITextListener_SMC
{
public:
	NavPlaceDatabaseLoader();
	~NavPlaceDatabaseLoader();

	bool ParseFile(const std::filesystem::path& path);

	std::unordered_map<std::string, std::string>& GetEntries() { return entries; }

private:

	/**
	 * @brief Called when starting parsing.
	 */
	void ReadSMC_ParseStart() override {}

	/**
	 * @brief Called when ending parsing.
	 *
	 * @param halted			True if abnormally halted, false otherwise.
	 * @param failed			True if parsing failed, false otherwise.
	 */
	void ReadSMC_ParseEnd(bool halted, bool failed) override {}

	/**
	 * @brief Called when entering a new section
	 *
	 * @param states		Parsing states.
	 * @param name			Name of section, with the colon omitted.
	 * @return				SMCResult directive.
	 */
	SourceMod::SMCResult ReadSMC_NewSection(const SourceMod::SMCStates* states, const char* name) override;

	/**
	 * @brief Called when encountering a key/value pair in a section.
	 *
	 * @param states		Parsing states.
	 * @param key			Key string.
	 * @param value			Value string.  If no quotes were specified, this will be NULL,
	 *						and key will contain the entire string.
	 * @return				SMCResult directive.
	 */
	SourceMod::SMCResult ReadSMC_KeyValue(const SourceMod::SMCStates* states, const char* key, const char* value) override;

	/**
	 * @brief Called when leaving the current section.
	 *
	 * @param states		Parsing states.
	 * @return				SMCResult directive.
	 */
	SourceMod::SMCResult ReadSMC_LeavingSection(const SourceMod::SMCStates* states) override;

	/**
	 * @brief Called after an input line has been preprocessed.
	 *
	 * @param states		Parsing states.
	 * @param line			Contents of the line, null terminated at the position
	 * 						of the newline character (thus, no newline will exist).
	 * @return				SMCResult directive.
	 */
	SourceMod::SMCResult ReadSMC_RawLine(const SourceMod::SMCStates* states, const char* line) override { return SourceMod::SMCResult_Continue; }

private:
	SourceMod::SMCStates parser_states;
	std::unordered_map<std::string, std::string> entries; // no duplicates allowed
	bool valid;
};

NavPlaceDatabaseLoader::NavPlaceDatabaseLoader()
{
	parser_states.col = 0;
	parser_states.line = 0;
	entries.reserve(1024);
	valid = false;
}

inline NavPlaceDatabaseLoader::~NavPlaceDatabaseLoader()
{
	entries.clear();
}

inline bool NavPlaceDatabaseLoader::ParseFile(const std::filesystem::path& path)
{
	if (!std::filesystem::exists(path))
	{
		return false;
	}

	valid = false;
	parser_states.col = 0;
	parser_states.line = 0;
	SourceMod::SMCError result = textparsers->ParseFile_SMC(path.string().c_str(), this, &parser_states);

	if (result != SourceMod::SMCError_Okay)
	{
		smutils->LogError(myself, "NavPlaceDatabaseLoader::ParseFile - Failed to parse \"%s\". Line %i Col %i ", path.string().c_str(), parser_states.line, parser_states.col);

		return false;
	}

	return true;
}

inline SourceMod::SMCResult NavPlaceDatabaseLoader::ReadSMC_NewSection(const SourceMod::SMCStates* states, const char* name)
{
	if (!valid)
	{
		if (strncasecmp(name, "NavPlaces", 9) == 0)
		{
			valid = true;
			return SourceMod::SMCResult_Continue;
		}
	}

	smutils->LogError(myself, "NavPlaceDatabaseLoader::ReadSMC_NewSection Unknown Section %s line %i col %i", name, states->line, states->col);
	return SourceMod::SMCResult_HaltFail;
}

inline SourceMod::SMCResult NavPlaceDatabaseLoader::ReadSMC_KeyValue(const SourceMod::SMCStates* states, const char* key, const char* value)
{
	if (std::strlen(key) <= 2 || std::strlen(value) <= 2)
	{
		smutils->LogError(myself, "NavPlaceDatabaseLoader ignoring entry <%s, %s> @ line %i col %i", key, value, states->line, states->col);
		return SourceMod::SMCResult_Continue;
	}

	if (valid)
	{
		std::string sKey{ key };
		std::string sValue{ value };

		if (entries.count(sKey) > 0)
		{
			smutils->LogError(myself, "NavPlaceDatabaseLoader ignoring duplicate entry <%s, %s> @ line %i col %i", key, value, states->line, states->col);
			return SourceMod::SMCResult_Continue;
		}

		entries[sKey] = sValue;
	}

	return SourceMod::SMCResult_Continue;
}

inline SourceMod::SMCResult NavPlaceDatabaseLoader::ReadSMC_LeavingSection(const SourceMod::SMCStates* states)
{
	return SourceMod::SMCResult_Continue;
}

#endif // !NAV_PLACE_LOADER_H_