/*
 * Handle.cpp
 *
 *  Created on: Apr 15, 2017
 */

#include "Handle.h"
#include <extension.h>

#include <eiface.h>

/*
IHandleEntity* CBaseHandle::Get() const
{
	extern CGlobalVars *gpGlobals;
	for (int i = 0; i < gpGlobals->maxEntities; i++) {
		extern IVEngineServer* engine;
		edict_t* ent = engine->PEntityOfEntIndex(i);
		if (ent != nullptr && m_Index == ent->m_NetworkSerialNumber) {
			return ent->GetIServerEntity();
		}
	}
	return nullptr;
}
*/

IHandleEntity* CBaseHandle::Get() const
{
	return g_EntList->LookupEntity(*this);
}
