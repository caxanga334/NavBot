#include <extension.h>
#include <gametrace.h>

bool CGameTrace::DidHitWorld() const
{
	int index = gamehelpers->EntityToBCompatRef(m_pEnt);
	return index == 0; // World entity is edict index 0
}

bool CGameTrace::DidHitNonWorldEntity() const
{
	return !DidHitWorld();
}

int CGameTrace::GetEntityIndex() const
{
	return gamehelpers->EntityToBCompatRef(m_pEnt);
}