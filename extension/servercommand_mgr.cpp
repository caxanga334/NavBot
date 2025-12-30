#include NAVBOT_PCH_FILE
#include "servercommand_mgr.h"

CServerCommandManager::CServerCommandManager()
{
}

CServerCommandManager::~CServerCommandManager()
{
	for (ConCommand* ptr : m_cmdptrs)
	{
		delete ptr;
	}
}

void CServerCommandManager::RegisterConCommand(const char* name, const char* description, int flags, ConCmdCallback callback, bool listenserveronly)
{
	if (listenserveronly && engine->IsDedicatedServer())
	{
		return;
	}

	
	std::string szName{ name };

	if (m_commands.find(szName) != m_commands.end())
	{
		EXT_ASSERT(false, "CServerCommandManager::RegisterConCommand trying to register duplicate command!");
		return;
	}

	SVConCmd cmd;
	cmd.m_callback = callback;
	ConCommand* ptr = new ConCommand(name, CServerCommandManager::OnCommandCallback, description, flags);
	m_commands[szName] = cmd;
	m_cmdptrs.push_back(ptr);
}

void CServerCommandManager::RegisterConCommandAutoComplete(const char* name, const char* description, int flags, ConCmdCallback callback, ConCmdAutoComplete autocomplete, bool listenserveronly)
{
	if (listenserveronly && engine->IsDedicatedServer())
	{
		return;
	}

	std::string szName{ name };

	if (m_commands.find(szName) != m_commands.end())
	{
		EXT_ASSERT(false, "CServerCommandManager::RegisterConCommand trying to register duplicate command!");
		return;
	}

	SVConCmd cmd;
	cmd.m_callback = callback;
	cmd.m_autocomplete = autocomplete;
	ConCommand* ptr = new ConCommand(name, CServerCommandManager::OnCommandCallback, description, flags, CServerCommandManager::OnCommandAutoComplete);
	m_commands[szName] = cmd;
	m_cmdptrs.push_back(ptr);
}

void CServerCommandManager::OnCommandCallback(const CCommand& command)
{
	std::string name{ command.Arg(0) };
	CServerCommandManager& manager = extmanager->GetServerCommandManager();

	auto it = manager.m_commands.find(name);

	if (it != manager.m_commands.end())
	{
		SVCommandArgs args;

		// push arguments
		for (int i = 0; i < command.ArgC(); i++)
		{
			args.PushArg(command.Arg(i));
		}

		it->second.m_callback(args);
	}
}

int CServerCommandManager::OnCommandAutoComplete(const char* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	if (V_strlen(partial) >= COMMAND_COMPLETION_ITEM_LENGTH)
	{
		return 0;
	}

	CServerCommandManager& manager = extmanager->GetServerCommandManager();
	char cmd[COMMAND_COMPLETION_ITEM_LENGTH + 2];
	V_strncpy(cmd, partial, sizeof(cmd));

	// skip to start of argument
	char* partialArg = V_strrchr(cmd, ' ');

	if (partialArg == NULL)
	{
		return 0;
	}

	// chop command from partial argument
	*partialArg = '\000';
	++partialArg;

	int partialArgLength = V_strlen(partialArg);

	std::string szName{ cmd };
	auto it = manager.m_commands.find(szName);

	if (it != manager.m_commands.end())
	{
		SVCommandAutoCompletion autocomplete;

		it->second.m_autocomplete(partialArg, partialArgLength, autocomplete);

		auto& entries = autocomplete.GetEntries();

		if (!entries.empty())
		{
			int c = 0;

			for (auto& entry : entries)
			{
				V_snprintf(commands[c], COMMAND_COMPLETION_ITEM_LENGTH, "%s %s", cmd, entry.c_str());

				if (++c >= COMMAND_COMPLETION_MAXITEMS)
				{
					return c;
				}
			}

			return c;
		}
	}

	return 0;
}
