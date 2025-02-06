#ifndef NAVBOT_CON_COMMANDS_ARGS_EPISODE1_COMPAT_H_
#define NAVBOT_CON_COMMANDS_ARGS_EPISODE1_COMPAT_H_

#include <memory>
#include <extension.h>

#if SOURCE_ENGINE == SE_EPISODEONE

// Episode 1/Source 2006 lacks CCommand args parameter for console commands, this macros creates one and does nothing on SDK branches that already have CCommand args
#define DECLARE_COMMAND_ARGS	\
	CCommand args;				\
								\

/**
 * @brief Port of CCommand for the Source 2006 engine. Translates to the original engine command argument handling.
 */
class CCommand
{
public:
	CCommand()
	{
		m_args = std::make_unique<char[]>(4096);
	}

	void Reset()
	{

	}

	int ArgC() const { return engine->Cmd_Argc(); }
	const char** ArgV() const
	{
		// TO-DO:
		return nullptr;
	}

	// All args that occur after the 0th arg, in string form
	const char* ArgS() const
	{
		for (int i = 1; i < engine->Cmd_Argc(); i++)
		{
			const char* arg = Arg(i);
			ke::SafeStrcat(m_args.get(), 4096, arg);
			// TO-DO: Test if we need to add spaces between args here
		}

		return m_args.get();
	}

	// // The entire command in string form, including the 0th arg
	const char* GetCommandString() const
	{
		return engine->Cmd_Args();
	}

	// Gets at arguments
	const char* operator[](int nIndex) const
	{
		return this->Arg(nIndex);
	}

	// Gets at arguments
	const char* Arg(int nIndex) const
	{
		// FIXME: Many command handlers appear to not be particularly careful
		// about checking for valid argc range. For now, we're going to
		// do the extra check and return an empty string if it's out of range
		if (nIndex < 0 || nIndex >= engine->Cmd_Argc())
			return "";

		return engine->Cmd_Argv(nIndex);
	}

	// Helper functions to parse arguments to commands.
	const char* FindArg(const char* pName) const;
	int FindArgInt(const char* pName, int nDefaultVal) const;

private:
	std::unique_ptr<char[]> m_args; // string for ArgS
};

inline const char* CCommand::FindArg(const char* pName) const
{
	int nArgC = ArgC();
	for (int i = 1; i < nArgC; i++)
	{
		if (!Q_stricmp(Arg(i), pName))
			return (i + 1) < nArgC ? Arg(i + 1) : "";
	}

	return nullptr;
}

inline int CCommand::FindArgInt(const char* pName, int nDefaultVal) const
{
	const char* pVal = FindArg(pName);
	if (pVal)
		return atoi(pVal);
	else
		return nDefaultVal;
}

#else

#define DECLARE_COMMAND_ARGS

#endif // SOURCE_ENGINE == SE_EPISODEONE



#endif // !NAVBOT_CON_COMMANDS_ARGS_EPISODE1_COMPAT_H_

