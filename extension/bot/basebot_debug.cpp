#include <extension.h>
#include <manager.h>
#include <sdkports/debugoverlay_shared.h>
#include "basebot.h"

static void OnBotDebugFilterChanged(IConVar* var, const char* pOldValue, float flOldValue);

ConVar cvar_bot_debug_filter("sm_navbot_debug_filter", "0", FCVAR_CHEAT | FCVAR_GAMEDLL | FCVAR_DONTRECORD, "Bot client index filter when debugging.", OnBotDebugFilterChanged);

static int s_bot_debug_filter;

static void OnBotDebugFilterChanged(IConVar* var, const char* pOldValue, float flOldValue)
{
	s_bot_debug_filter = cvar_bot_debug_filter.GetInt();
}

bool CBaseBot::IsDebugging(int bits) const
{
	if (s_bot_debug_filter > 0 && s_bot_debug_filter != GetIndex())
	{
		return false;
	}

	return extmanager->IsDebugging(bits);
}

const char* CBaseBot::GetDebugIdentifier() const
{
	static char debug[256];

	auto player = playerhelpers->GetGamePlayer(GetIndex());

	ke::SafeSprintf(debug, sizeof(debug), "%3.2f: %s#%i/%i", gpGlobals->curtime, player->GetName(), GetIndex(), player->GetUserId());

	return debug;
}

void CBaseBot::DebugPrintToConsole(const int bits,const int red,const int green,const int blue, const char* fmt, ...) const
{
	if (!IsDebugging(bits))
	{
		return;
	}

	char buffer[512]{};
	va_list vaargs;
	va_start(vaargs, fmt);
	size_t len = ke::SafeVsprintf(buffer, sizeof(buffer), fmt, vaargs);

	if (len >= sizeof(buffer) - 1)
	{
		buffer[511] = '\0';
	}
	else {
		buffer[len] = '\0';
	}

	va_end(vaargs);

	ConColorMsg(Color(red, green, blue, 255), "%s", buffer);
}

void CBaseBot::DebugDisplayText(const char* text)
{
	NDebugOverlay::EntityText(GetIndex(), m_debugtextoffset, text, 0.1f);
	m_debugtextoffset++;
}

void CBaseBot::DebugFrame()
{
	char text[8];
	ke::SafeSprintf(text, sizeof(text), "#%i", GetIndex());
	DebugDisplayText(text);
}
