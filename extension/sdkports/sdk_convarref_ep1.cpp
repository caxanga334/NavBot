#include NAVBOT_PCH_FILE
#include <extension.h>
#include "sdk_convarref_ep1.h"

#if SOURCE_ENGINE == SE_EPISODEONE

ConVarRef::ConVarRef(const char* name)
{
	m_pCvar = icvar->FindVar(name);

#ifdef EXT_DEBUG
	if (!m_pCvar)
	{
		smutils->LogError(myself, "[ConVarRef EP1 COMPAT] ConVar \"%s\" not found!", name);
	}
#endif // EXT_DEBUG
}

ConVarRef::ConVarRef(const char* name, const bool ignoreMissing)
{
	m_pCvar = icvar->FindVar(name);

	if (!ignoreMissing)
	{
		smutils->LogError(myself, "[ConVarRef EP1 COMPAT] ConVar \"%s\" not found!", name);
	}
}

const char* ConVarRef::GetString(void) const
{
	return m_pCvar->GetString();
}

float ConVarRef::GetFloat(void) const
{
	return m_pCvar->GetFloat();
}

int ConVarRef::GetInt(void) const
{
	return m_pCvar->GetInt();
}

void ConVarRef::SetValue(const char* pValue)
{
	m_pCvar->SetValue(pValue);
}

void ConVarRef::SetValue(float flValue)
{
	m_pCvar->SetValue(flValue);
}

void ConVarRef::SetValue(int nValue)
{
	m_pCvar->SetValue(nValue);
}

void ConVarRef::SetValue(bool bValue)
{
	m_pCvar->SetValue(bValue ? 1 : 0);
}

#endif