#include NAVBOT_PCH_FILE
#include "concmd_args.h"

CConCommandArgs::CConCommandArgs()
{
}

bool CConCommandArgs::HasArg(const char* arg) const
{
	for (auto& string : m_args)
	{
		const char* ptr = string.c_str();

		if (ke::StrCaseCmp(ptr, arg) == 0)
		{
			return true;
		}
	}

	return false;
}

bool CConCommandArgs::IsCommand(const char* szCmdName) const
{
	return ke::StrCaseCmp(m_args[0].c_str(), szCmdName) == 0;
}

const char* CConCommandArgs::FindArg(const char* arg, const char* def) const
{
	for (std::size_t i = 1; i < m_args.size(); i++)
	{
		const char* ptr = m_args[i].c_str();

		// found argument
		if (ke::StrCaseCmp(ptr, arg) == 0)
		{
			std::size_t valueidx = i + 1;

			if (valueidx >= m_args.size())
			{
				return def;
			}

			return m_args[valueidx].c_str();
		}
	}

	return def;
}

int CConCommandArgs::FindArgInt(const char* arg, int defaultValue) const
{
	const char* szValue = FindArg(arg);

	if (szValue)
	{
		return atoi(arg);
	}

	return defaultValue;
}

float CConCommandArgs::FindArgFloat(const char* arg, float defaultValue) const
{
	const char* szValue = FindArg(arg);

	if (szValue)
	{
		return atof(arg);
	}

	return defaultValue;
}

std::uint64_t CConCommandArgs::FindArgUint64(const char* arg, std::uint64_t defaultValue) const
{
	for (std::size_t i = 1; i < m_args.size(); i++)
	{
		const char* ptr = m_args[i].c_str();

		// found argument
		if (ke::StrCaseCmp(ptr, arg) == 0)
		{
			std::size_t valueidx = i + 1;

			if (valueidx >= m_args.size())
			{
				return defaultValue;
			}

			return static_cast<std::uint64_t>(std::stoull(m_args[valueidx]));
		}
	}

	return defaultValue;
}

bool CConCommandArgs::ParseArgVector(const char* arg, Vector& out) const
{
	const char* szValue = FindArg(arg);

	if (szValue)
	{
		int n = sscanf(szValue, "%f %f %f", &out.x, &out.y, &out.z);

		return n == 3;
	}

	return false;
}
