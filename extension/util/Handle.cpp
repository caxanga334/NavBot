/*
 * Handle.cpp
 *
 *  Created on: Apr 15, 2017
 */

#include "Handle.h"

#include <eiface.h>

IHandleEntity* CBaseHandle::Get() const
{
	extern CGlobalVars *gpGlobals;
	for (unsigned int i = 0; i < gpGlobals->maxEntities; i++) {
		extern IVEngineServer* engine;
		edict_t* ent = engine->PEntityOfEntIndex(i);
		if (ent != nullptr && m_Index == ent->m_NetworkSerialNumber) {
			return ent->GetIServerEntity();
		}
	}
	return nullptr;
}
