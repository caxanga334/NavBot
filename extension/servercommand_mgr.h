#ifndef __NAVBOT_SERVER_COMMAND_MANAGER_H_
#define __NAVBOT_SERVER_COMMAND_MANAGER_H_

#include "util/concmd_args.h"

class ConCommand;

class CServerCommandManager
{
public:
	// void (const CConCommandArgs& args)
	using ConCmdCallback = std::function<void(const CConCommandArgs&)>;
	// void (const char* partialArg, int partialArgLength, SVCommandAutoCompletion& entries)
	using ConCmdAutoComplete = std::function<void(const char*,int,SVCommandAutoCompletion&)>;
	// void (ConVar* pCvar, const char* pOldValue)
	using ConVarChangedCallback = std::function<void(ConVar*, const char*)>;
	using ConVarPair = std::pair<ConVar*, std::vector<ConVarChangedCallback>>;

	CServerCommandManager();
	~CServerCommandManager();

	/**
	 * @brief Registers a new server side console command.
	 * @param name Command name. (Must point to static memory)
	 * @param description Command description. (Must point to static memory)
	 * @param flags Command flags.
	 * @param callback Callback function to call when the command is used.
	 * @param listenserveronly If true, only registers this command if running on a listen server.
	 */
	void RegisterConCommand(const char* name, const char* description, int flags, ConCmdCallback callback, bool listenserveronly = false);
	/**
	 * @brief Registers a new server side console command with auto completion.
	 * @param name Command name. (Must point to static memory)
	 * @param description Command description. (Must point to static memory)
	 * @param flags Command flags.
	 * @param callback Callback function to call when the command is used.
	 * @param autocomplete Callback function called to auto complete the command.
	 * @param listenserveronly If true, only registers this command if running on a listen server.
	 */
	void RegisterConCommandAutoComplete(const char* name, const char* description, int flags, ConCmdCallback callback, ConCmdAutoComplete autocomplete, bool listenserveronly = false);
	/**
	 * @brief Register a console variable.
	 * @param name ConVar name.
	 * @param description ConVar description.
	 * @param defaultValue ConVar default value.
	 * @param flags ConVar flags.
	 * @return Pointer to the ConVar just registered.
	 */
	ConVar* RegisterConVar(const char* name, const char* description, const char* defaultValue, int flags);
	/**
	 * @brief Register a console variable.
	 * @param name ConVar name.
	 * @param description ConVar description.
	 * @param defaultValue ConVar default value.
	 * @param flags ConVar flags.
	 * @param changecallback Callback function for when the ConVar has changed values.
	 * @return Pointer to the ConVar just registered.
	 */
	ConVar* RegisterConVar(const char* name, const char* description, const char* defaultValue, int flags, ConVarChangedCallback changecallback);
	/**
	 * @brief Searches for a ConVar by it's name.
	 * @param name ConVar name to search for.
	 * @return Pointer to the ConVar or NULL if not found.
	 */
	ConVar* FindVar(const char* name) const;
private:
	class SVConCmd
	{
	public:

		ConCmdCallback m_callback;
		ConCmdAutoComplete m_autocomplete;
	};

	std::unordered_map<std::string, SVConCmd> m_commands;
	std::vector<ConCommand*> m_cmdptrs;
	std::unordered_map<std::string, ConVarPair> m_convars;

#if SOURCE_ENGINE > SE_DARKMESSIAH
	static void OnCommandCallback(const CCommand& command);
#else
	static void OnCommandCallback(void);
#endif // SOURCE_ENGINE > SE_DARKMESSIAH

#if SOURCE_ENGINE > SE_DARKMESSIAH
	static void OnConVarChanged(IConVar* var, const char* pOldValue, float flOldValue);
#else
	static void OnConVarChanged(ConVar* var, char const* pOldString);
#endif // SOURCE_ENGINE > SE_DARKMESSI
	
	static int OnCommandAutoComplete(const char* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);

	const ConVarPair* FindConVarPair(const char* name) const;
};



#endif // !__NAVBOT_SERVER_COMMAND_MANAGER_H_
