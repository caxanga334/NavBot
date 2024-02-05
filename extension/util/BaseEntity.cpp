/*
 * BaseClass.cpp
 *
 *  Created on: Apr 6, 2017
 */

#include "BaseEntity.h"

#include <extension.h>
#include "SimpleException.h"
#include <eiface.h>
#include <server_class.h>
#include <const.h>

bool BaseEntity::isDestroyedOrUsed() const {
	return (get<int>("m_fEffects", EF_NODRAW) & EF_NODRAW) > 0;
}

edict_t* BaseEntity::getEntity(const char *varName) const {
	CBaseHandle* out = getPtr<CBaseHandle>(varName);
	extern IVEngineServer* engine;
	return out == nullptr ?
		nullptr : gamehelpers->EdictOfIndex(out->GetEntryIndex());
}

char *BaseEntity::getPointer(const char* varName) const {
	if (ent == nullptr) {
		return nullptr;
	}
	IServerUnknown *unk = ent->GetUnknown();
	if (ent->IsFree() || !unk || !ent->m_pNetworkable) {
		return nullptr;
	}
	ServerClass *serverClass = ent->m_pNetworkable->GetServerClass();
	int offset = getOffset(varName, serverClass->m_pTable, 0);
	return offset < 0 ? nullptr : reinterpret_cast<char *>(unk->GetBaseEntity()) + offset;
}

int BaseEntity::getOffset(const char* varName, SendTable* pTable, int offset) {
	for (int i = 0; i < pTable->GetNumProps(); i++) {
		SendProp *prop = pTable->GetProp(i);
		const char *pname = prop->GetName();
		if (pname != nullptr && strcmp(varName, pname) == 0) {
			// for some reason offset is sometimes negative when it shouldn't be
			// so take the absolute value
			return offset + abs(prop->GetOffset());
		}
		if (prop->GetDataTable()) {
			int newOffset = getOffset(varName, prop->GetDataTable(),
						offset + abs(prop->GetOffset()));
			if (newOffset >= 0) {
				return newOffset;
			}
		}
	}
	return -1;
}

