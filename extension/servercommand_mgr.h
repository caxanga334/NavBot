#ifndef __NAVBOT_SERVER_COMMAND_MANAGER_H_
#define __NAVBOT_SERVER_COMMAND_MANAGER_H_

#include "util/concmd_args.h"

class ConCommand;

class CServerCommandManager
{
public:
	// void (const SVCommandArgs& args)
	using ConCmdCallback = std::function<void(const SVCommandArgs&)>;
	// void (const char* partialArg, int partialArgLength, SVCommandAutoCompletion& entries)
	using ConCmdAutoComplete = std::function<void(const char*,int,SVCommandAutoCompletion&)>;

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

private:
	class SVConCmd
	{
	public:

		ConCmdCallback m_callback;
		ConCmdAutoComplete m_autocomplete;
	};

	std::unordered_map<std::string, SVConCmd> m_commands;
	std::vector<ConCommand*> m_cmdptrs;

#if SOURCE_ENGINE > SE_DARKMESSIAH
	static void OnCommandCallback(const CCommand& command);
#else
	static void OnCommandCallback(void);
#endif // SOURCE_ENGINE > SE_DARKMESSIAH

	
	static int OnCommandAutoComplete(const char* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);
};



#endif // !__NAVBOT_SERVER_COMMAND_MANAGER_H_
