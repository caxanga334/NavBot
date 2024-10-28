#ifndef NAVBOT_CORE_BAD_PLUGINS_H_
#define NAVBOT_CORE_BAD_PLUGINS_H_

#include <string>
#include <vector>

#include <ITextParsers.h>
#include <IPluginSys.h>

class CBadPluginChecker final : public SourceMod::ITextListener_SMC
{
public:
	CBadPluginChecker();
	~CBadPluginChecker();

	enum Action : int
	{
		ACTION_LOG = 0, // Log an error to Sourcemod's logs
		ACTION_EVICT, // Unload the plugin

		MAX_ACTION_TYPES
	};

	enum MatchType : int
	{
		MATCH_FULL = 0, // Match full name
		MATCH_PARTIAL, // Partial match

		MAX_MATCH_TYPES
	};

	class ConfigEntry
	{
	public:

		ConfigEntry()
		{
			action = ACTION_LOG;
			matchType = MATCH_FULL;
		}

		std::string entryName; // config entry name
		Action action; // action to take
		std::string matchString; // string to match
		MatchType matchType; // match type
	};

	bool LoadConfigFile(char* error, size_t maxlen);

	void ValidatePlugin(IPlugin* plugin) const;

	/**
	 * @brief Called when entering a new section
	 *
	 * @param states		Parsing states.
	 * @param name			Name of section, with the colon omitted.
	 * @return				SMCResult directive.
	 */
	virtual SourceMod::SMCResult ReadSMC_NewSection(const SourceMod::SMCStates* states, const char* name) final;

	/**
	 * @brief Called when encountering a key/value pair in a section.
	 *
	 * @param states		Parsing states.
	 * @param key			Key string.
	 * @param value			Value string.  If no quotes were specified, this will be NULL,
	 *						and key will contain the entire string.
	 * @return				SMCResult directive.
	 */
	virtual SourceMod::SMCResult ReadSMC_KeyValue(const SourceMod::SMCStates* states, const char* key, const char* value) final;

	/**
		* @brief Called when leaving the current section.
		*
		* @param states		Parsing states.
		* @return				SMCResult directive.
		*/
	SourceMod::SMCResult ReadSMC_LeavingSection(const SourceMod::SMCStates* states) final;

private:
	std::vector<ConfigEntry> cfg_entries;
	unsigned int parser_depth;
	ConfigEntry* current;

	void TakeActionAgainstPlugin(IPlugin* plugin, Action action) const;
	void RemoveBadEntries();
};

#endif // !NAVBOT_CORE_BAD_PLUGINS_H_
