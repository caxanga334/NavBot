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

	for (auto& pair : m_convars)
	{
		delete pair.second.first;
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

ConVar* CServerCommandManager::RegisterConVar(const char* name, const char* description, const char* defaultValue, int flags)
{
	std::string szName{ name };

	if (m_convars.find(szName) != m_convars.end())
	{
		EXT_ASSERT(false, "CServerCommandManager::RegisterConVar trying to register duplicate ConVar!");
		return nullptr;
	}

	ConVar* var = new ConVar(name, defaultValue, flags, description, CServerCommandManager::OnConVarChanged);
	ConVarPair tmp;

	auto result = m_convars.emplace(std::make_pair(szName, tmp));

	if (!result.second)
	{
		EXT_ASSERT(false, "CServerCommandManager::RegisterConVar failed to create ConVarPair!");
		return nullptr;
	}

	auto& element = *result.first;
	element.second.first = var;
	
	return var;
}

ConVar* CServerCommandManager::RegisterConVar(const char* name, const char* description, const char* defaultValue, int flags, ConVarChangedCallback changecallback)
{
	RegisterConVar(name, description, defaultValue, flags);

	std::string szName{ name };
	auto it = m_convars.find(szName);

	if (it == m_convars.end())
	{
		EXT_ASSERT(false, "CServerCommandManager::RegisterConVar ConVar not found to add change callback!");
		return nullptr;
	}

	it->second.second.emplace_back(changecallback);
	return it->second.first;
}

ConVar* CServerCommandManager::FindVar(const char* name) const
{
	std::string szName{ name };
	auto it = m_convars.find(szName);

	if (it == m_convars.end())
	{
		return nullptr;
	}

	return it->second.first;
}

#if SOURCE_ENGINE > SE_DARKMESSIAH

void CServerCommandManager::OnCommandCallback(const CCommand& command)
{
	std::string name{ command.Arg(0) };
	CServerCommandManager& manager = extmanager->GetServerCommandManager();

	auto it = manager.m_commands.find(name);

	if (it != manager.m_commands.end())
	{
		CConCommandArgs args;

		// push arguments
		for (int i = 0; i < command.ArgC(); i++)
		{
			args.PushArg(command.Arg(i));
		}

		it->second.m_callback(args);
	}
}

void CServerCommandManager::OnConVarChanged(IConVar* var, const char* pOldValue, float flOldValue)
{
	CServerCommandManager& manager = extmanager->GetServerCommandManager();
	const ConVarPair* pair = manager.FindConVarPair(var->GetName());

	if (pair)
	{
		for (auto& func : pair->second)
		{
			func(pair->first, pOldValue);
		}
	}
}

#else

// Old Engine games ( Source 2006 )
void CServerCommandManager::OnCommandCallback(void)
{

	std::string name{ engine->Cmd_Argv(0) };
	CServerCommandManager& manager = extmanager->GetServerCommandManager();

	auto it = manager.m_commands.find(name);

	if (it != manager.m_commands.end())
	{
		CConCommandArgs args;

		// push arguments
		for (int i = 0; i < engine->Cmd_Argc(); i++)
		{
			args.PushArg(engine->Cmd_Argv(i));
		}

		it->second.m_callback(args);
	}
}

void CServerCommandManager::OnConVarChanged(ConVar* var, char const* pOldString)
{
	CServerCommandManager& manager = extmanager->GetServerCommandManager();
	const ConVarPair* pair = manager.FindConVarPair(var->GetName());

	if (pair)
	{
		for (auto& func : pair->second)
		{
			func(pair->first, pOldString);
		}
	}
}

#endif // SOURCE_ENGINE > SE_DARKMESSIAH

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

const CServerCommandManager::ConVarPair* CServerCommandManager::FindConVarPair(const char* name) const
{
	std::string szName{ name };
	auto it = m_convars.find(szName);

	if (it == m_convars.end())
	{
		return nullptr;
	}

	return &(it->second);
}
