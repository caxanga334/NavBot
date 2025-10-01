/**
 * vim: set ts=4 :
 * =============================================================================
 * Copyright (C) 2004-2010 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 */

#include NAVBOT_PCH_FILE
#include <exception>
#include <sdkports/sdk_entityoutput.h>
#include "gamedata_const.h"
#include "entprops.h"

#ifdef EXT_VPROF_ENABLED
#include <tier0/vprof.h>
#endif // EXT_VPROF_ENABLED

enum PropEntType
{
	PropEnt_Unknown,
	PropEnt_Handle,
	PropEnt_Entity,
	PropEnt_Edict,
	PropEnt_Variant,
};

static CEntPropUtils s_entprops;
CEntPropUtils *entprops = &s_entprops;

#define SET_TYPE_IF_VARIANT(type) \
	if (td->fieldType == FIELD_CUSTOM && (td->flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT) \
	{ \
		auto *pVariant = (variant_t *)((intptr_t)pEntity + offset); \
		pVariant->fieldType = type; \
	}

#define PROP_TYPE_SWITCH(type, type_name, returnval) \
	switch (pProp->GetType()) \
	{ \
	case type: \
	{ \
		if (element != 0) \
		{ \
			return returnval; \
		} \
		 \
		break; \
	} \
	case DPT_Array: \
	{ \
		int elementCount = pProp->GetNumElements(); \
		int elementStride = pProp->GetElementStride(); \
		if (element < 0 || element >= elementCount) \
		{ \
			return returnval; \
		} \
		\
		pProp = pProp->GetArrayProp(); \
		if (!pProp) { \
			return returnval; \
		} \
		\
		if (pProp->GetType() != type) \
		{ \
			return returnval; \
		} \
		\
		offset += pProp->GetOffset() + (elementStride * element); \
		bit_count = pProp->m_nBits; \
		break; \
	} \
	case DPT_DataTable: \
	{ \
		SendTable *pTable = pProp->GetDataTable(); \
		if (!pTable) \
		{ \
			return returnval; \
		} \
		\
		int elementCount = pTable->GetNumProps(); \
		if (element < 0 || element >= elementCount) \
		{ \
			return returnval; \
		} \
		\
		pProp = pTable->GetProp(element); \
		if (pProp->GetType() != type) \
		{ \
			if (pProp->GetType() != type) \
			{ \
				return returnval; \
			} \
		} \
		 \
		offset += pProp->GetOffset(); \
		bit_count = pProp->m_nBits; \
		break; \
	} \
	default: \
	{ \
		return returnval; \
		break; \
	} \
	}

#define CHECK_TYPE_VALID_IF_VARIANT(type, typeName, returnval) \
	if (td->fieldType == FIELD_CUSTOM && (td->flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT) \
	{ \
		auto *pVariant = (variant_t *)((intptr_t)pEntity + offset); \
		if (pVariant->fieldType != type) \
		{ \
			return returnval; \
		} \
	}

#define CHECK_SET_PROP_DATA_OFFSET(returnval) \
	if (element < 0 || element >= td->fieldSize) \
	{ \
		return returnval; \
	} \
	\
	offset = dinfo.actual_offset + (element * (td->fieldSizeInBytes / td->fieldSize));

#define GAMERULES_FIND_PROP_SEND(type, type_name, retval) \
	SourceMod::sm_sendprop_info_t info;\
	SendProp *pProp; \
	if (!gamehelpers->FindSendPropInfo(grclassname.c_str(), prop, &info)) \
	{ \
	\
		smutils->LogError(myself, "Game Rules property \"%s\" not found on the gamerules proxy.", prop); \
		return retval; \
	} \
	\
	offset = info.actual_offset; \
	pProp = info.prop; \
	bit_count = pProp->m_nBits; \
	\
	switch (pProp->GetType()) \
	{ \
	case type: \
		{ \
			if (element != 0) \
			{ \
				return retval; \
			} \
			break; \
		} \
	case DPT_Array: \
		{ \
			int elementCount = pProp->GetNumElements(); \
			int elementStride = pProp->GetElementStride(); \
			if (element < 0 || element >= elementCount) \
			{ \
				return retval; \
			} \
			\
			pProp = pProp->GetArrayProp(); \
			if (!pProp) { \
				return retval; \
			} \
			\
			if (pProp->GetType() != type) \
			{ \
				return retval; \
			} \
			\
			offset += pProp->GetOffset() + (elementStride * element); \
			bit_count = pProp->m_nBits; \
			break; \
		} \
	case DPT_DataTable: \
		{ \
			GAMERULES_FIND_PROP_SEND_IN_SENDTABLE(info, pProp, element, type, type_name, retval); \
			\
			offset += pProp->GetOffset(); \
			bit_count = pProp->m_nBits; \
			break; \
		} \
	default: \
		{ \
			return retval; \
		} \
	} \

#define GAMERULES_FIND_PROP_SEND_IN_SENDTABLE(info, pProp, element, type, type_name, retval) \
	SendTable *pTable = pProp->GetDataTable(); \
	if (!pTable) \
	{ \
		return retval; \
	} \
	\
	int elementCount = pTable->GetNumProps(); \
	if (element < 0 || element >= elementCount) \
	{ \
		return retval; \
	} \
	\
	pProp = pTable->GetProp(element); \
	if (pProp->GetType() != type) \
	{ \
		return retval; \
	}

CEntPropUtils::CEntPropUtils()
{
	grclassname.reserve(64);
	initialized = false;
	std::fill(std::begin(cached_offsets), std::end(cached_offsets), 0);
}

void CEntPropUtils::Init(bool reset)
{
	if (initialized && !reset)
		return;

	SourceMod::IGameConfig *gamedata;
	char *error = nullptr;
	size_t maxlength = 0;
	grclassname.clear();

	if (!gameconfs->LoadGameConfigFile("sdktools.games", &gamedata, error, maxlength))
	{
		smutils->LogError(myself, "CEntPropUtils::Init -- Failed to load sdktools.game from SourceMod gamedata.");
		return;
	}

	const char* value = gamedata->GetKeyValue("GameRulesProxy");

	if (!value)
	{
		smutils->LogError(myself, "Failed to get \"GameRulesProxy\" key from SDKTools's gamedata!");
	}
	else
	{
#ifdef EXT_DEBUG
		smutils->LogMessage(myself, "CEntPropUtils::Init -- Retrieved game rules proxy classname \"%s\".", value);
#endif // EXT_DEBUG
		grclassname.assign(value);
	}

	BuildCache();

	gameconfs->CloseGameConfigFile(gamedata);
	initialized = true;
#ifdef EXT_DEBUG
	smutils->LogMessage(myself, "CEntPropUtils::Init -- Done.");
#endif // EXT_DEBUG
}

/**
 * @brief Checks if a property exists on an entity.
 * @param entity Entity index/reference to search.
 * @param proptype Property type.
 * @param prop Property name.
 * @param offset Optional variable to store the property offset if found.
 * @return True if the property was found. False otherwise.
 */
bool CEntPropUtils::HasEntProp(int entity, PropType proptype, const char* prop, unsigned int* offset)
{
	CBaseEntity* pEntity = gamehelpers->ReferenceToEntity(entity);
	return HasEntProp(pEntity, proptype, prop, offset);
}

bool CEntPropUtils::HasEntProp(edict_t* entity, PropType proptype, const char* prop, unsigned int* offset)
{
	CBaseEntity* pEntity = UtilHelpers::EdictToBaseEntity(entity);
	return HasEntProp(pEntity, proptype, prop, offset);
}

bool CEntPropUtils::HasEntProp(CBaseEntity* entity, PropType proptype, const char* prop, unsigned int* offset)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CEntPropUtils::HasEntProp", "NavBot");
#endif // EXT_VPROF_ENABLED

	using namespace SourceMod;

	switch (proptype)
	{
	case Prop_Send:
	{
		ServerClass* pClass = gamehelpers->FindEntityServerClass(entity);
		sm_sendprop_info_t info;

		if (!pClass)
		{
			return false;
		}

		if (gamehelpers->FindSendPropInfo(pClass->GetName(), prop, &info))
		{
			if (offset != nullptr)
			{
				*offset = info.actual_offset;
			}

			return true;
		}

		break;
	}
	case Prop_Data:
	{
		datamap_t* map = gamehelpers->GetDataMap(entity);
		sm_datatable_info_t info;

		if (map == nullptr)
		{
			return false;
		}

		if (gamehelpers->FindDataMapInfo(map, prop, &info))
		{
			if (offset != nullptr)
			{
				*offset = info.actual_offset;
			}

			return true;
		}

		break;
	}
	default:
		return false;
	}


	return false;
}

/// @brief Checks if the given entity is a networked entity
/// @param pEntity Entity to check
/// @return true if the entity is networked, false otherwise
bool CEntPropUtils::IsNetworkedEntity(CBaseEntity *pEntity)
{
	IServerNetworkable* pNet = reinterpret_cast<IServerUnknown*>(pEntity)->GetNetworkable();

	if (!pNet)
	{
		return false;
	}

	edict_t* edict = pNet->GetEdict();
	return (edict && !edict->IsFree()) ? true : false;
}

bool CEntPropUtils::FindSendProp(SourceMod::sm_sendprop_info_t *info, CBaseEntity *pEntity, const char *prop)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CEntPropUtils::FindSendProp", "NavBot");
#endif // EXT_VPROF_ENABLED

	ServerClass* pServerClass = FindEntityServerClass(pEntity);

	if (pServerClass == nullptr)
	{
		return false;
	}

	if (!gamehelpers->FindSendPropInfo(pServerClass->GetName(), prop, info))
	{
#ifdef EXT_DEBUG
		// Log errors on debug so programmers can fix it if needed
		const char* classname = entityprops::GetEntityClassname(pEntity);
		int entindex = UtilHelpers::IndexOfEntity(pEntity);
		smutils->LogError(myself, "Entity #%i <%s> [%s] does not have networked property named \"%s\"!", entindex, classname, pServerClass->GetName(), prop);
#endif // EXT_DEBUG
		return false;
	}

	return true;
}

bool CEntPropUtils::FindDataMap(CBaseEntity* pEntity, SourceMod::sm_datatable_info_t& dinfo, const char* prop)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CEntPropUtils::FindDataMap", "NavBot");
#endif // EXT_VPROF_ENABLED

	datamap_t* pMap = gamehelpers->GetDataMap(pEntity);

	if (pMap == nullptr)
	{
		smutils->LogError(myself, "DataMap lookup failed for prop %s", prop);
		return false;
	}

	if (!gamehelpers->FindDataMapInfo(pMap, prop, &dinfo))
	{
#ifdef EXT_DEBUG
		// Log errors on debug so programmers can fix it if needed
		const char* classname = entityprops::GetEntityClassname(pEntity);
		smutils->LogError(myself, "Entity <%s> [%s] does not have datamap property named \"%s\"!", classname, pMap->dataClassName, prop);
#endif // EXT_DEBUG
		return false;
	}

	return true;
}

/* Given an entity reference or index, fill out a CBaseEntity and/or edict.
   If lookup is successful, returns true and writes back the two parameters.
   If lookup fails, returns false and doesn't touch the params.  */
bool CEntPropUtils::IndexToAThings(int num, CBaseEntity **pEntData, edict_t **pEdictData)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CEntPropUtils::IndexToAThings", "NavBot");
#endif // EXT_VPROF_ENABLED

	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(num);

	if (!pEntity)
	{
		return false;
	}

	int index = gamehelpers->ReferenceToIndex(num);
	if (index > 0 && index <= playerhelpers->GetMaxClients())
	{
		SourceMod::IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(index);
		if (!pPlayer || !pPlayer->IsConnected())
		{
			return false;
		}
	}

	if (pEntData)
	{
		*pEntData = pEntity;
	}

	if (pEdictData)
	{
		edict_t *pEdict = BaseEntityToEdict(pEntity);
		if (!pEdict || pEdict->IsFree())
		{
			pEdict = nullptr;
		}

		*pEdictData = pEdict;
	}

	return true;
}

/**
 * @brief Retrieves an integer value in an entity's property.
 * @param entity Entity/edict index.
 * @param proptype Property type.
 * @param prop Property name.
 * @param result The lookup result will be stored here
 * @param size Number of bytes to write (valid values are 1, 2, or 4). This value is auto-detected, and the size parameter is only used as a fallback in case detection fails.
 * @param element Element # (starting from 0) if property is an array.
 * @return TRUE on success, FALSE on failure
*/
bool CEntPropUtils::GetEntProp(int entity, PropType proptype, const char *prop, int& result, int size, int element)
{
	CBaseEntity* pEntity = gamehelpers->ReferenceToEntity(entity);
	return GetEntProp(pEntity, proptype, prop, result, size, element);
}

bool CEntPropUtils::GetEntProp(edict_t* entity, PropType proptype, const char* prop, int& result, int size, int element)
{
	CBaseEntity* pEntity = UtilHelpers::EdictToBaseEntity(entity);
	return GetEntProp(pEntity, proptype, prop, result, size, element);
}

bool CEntPropUtils::GetEntProp(CBaseEntity* pEntity, PropType proptype, const char* prop, int& result, int size, int element)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CEntPropUtils::GetEntProp", "NavBot");
#endif // EXT_VPROF_ENABLED

	edict_t* pEdict = UtilHelpers::BaseEntityToEdict(pEntity);
	SourceMod::sm_sendprop_info_t info;
	SendProp* pProp = nullptr;
	int bit_count;
	int offset;
	bool is_unsigned = false;

	switch (proptype)
	{
	case Prop_Data:
		typedescription_t* td;
		SourceMod::sm_datatable_info_t dinfo;

		if (!FindDataMap(pEntity, dinfo, prop))
		{
			return false;
		}

		td = dinfo.prop;

		if ((bit_count = MatchTypeDescAsInteger(td->fieldType, td->flags)) == 0)
		{
			return false;
		}

		CHECK_SET_PROP_DATA_OFFSET(false);

		if (td->fieldType == FIELD_CUSTOM && (td->flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT)
		{
			auto* pVariant = (variant_t*)((intptr_t)pEntity + offset);
			if ((bit_count = MatchTypeDescAsInteger(pVariant->fieldType, 0)) == 0)
			{
				return false;
			}
		}

		break;

	case Prop_Send:

		if (!FindSendProp(&info, pEntity, prop))
		{
			return false;
		}

		offset = info.actual_offset;
		pProp = info.prop;
		bit_count = pProp->m_nBits;

		PROP_TYPE_SWITCH(DPT_Int, "integer", false);

#if SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS \
			|| SOURCE_ENGINE == SE_BMS || SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_TF2 \
			|| SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE || SOURCE_ENGINE == SE_PVKII
		if (pProp->GetFlags() & SPROP_VARINT)
		{
			bit_count = sizeof(int) * 8;
		}
#endif

		is_unsigned = ((pProp->GetFlags() & SPROP_UNSIGNED) == SPROP_UNSIGNED);

		break;

	default:
		return false;
		break;
	}

	if (bit_count < 1)
	{
		bit_count = size * 8;
	}

	if (bit_count >= 17)
	{
		result = *(int32_t*)((uint8_t*)pEntity + offset);
		return true;
	}
	else if (bit_count >= 9)
	{
		if (is_unsigned)
		{
			result = *(uint16_t*)((uint8_t*)pEntity + offset);
			return true;
		}
		else
		{
			result = *(int16_t*)((uint8_t*)pEntity + offset);
			return true;
		}
	}
	else if (bit_count >= 2)
	{
		if (is_unsigned)
		{
			result = *(uint8_t*)((uint8_t*)pEntity + offset);
			return true;
		}
		else
		{
			result = *(int8_t*)((uint8_t*)pEntity + offset);
			return true;
		}
	}
	else
	{
		result = *(bool*)((uint8_t*)pEntity + offset) ? 1 : 0;
		return true;
	}
}

/**
 * @brief Retrieves a boolean value in an entity's property.
 * @param entity Entity/edict index.
 * @param proptype Property type.
 * @param prop Property name.
 * @param result The lookup result will be stored here
 * @param element Element # (starting from 0) if property is an array.
 * @return 
*/
bool CEntPropUtils::GetEntPropBool(int entity, PropType proptype, const char* prop, bool& result, int element)
{
	int val = 0;
	bool retv = GetEntProp(entity, proptype, prop, val, 1, element);
	result = val != 0;
	return retv;
}

bool CEntPropUtils::GetEntPropBool(edict_t* entity, PropType proptype, const char* prop, bool& result, int element)
{
	int val = 0;
	bool retv = GetEntProp(entity, proptype, prop, val, 1, element);
	result = val != 0;
	return retv;
}

bool CEntPropUtils::GetEntPropBool(CBaseEntity* entity, PropType proptype, const char* prop, bool& result, int element)
{
	int val = 0;
	bool retv = GetEntProp(entity, proptype, prop, val, 1, element);
	result = val != 0;
	return retv;
}

/// @brief Sets an integer value in an entity's property.
/// @param entity Entity/edict index.
/// @param proptype Property type.
/// @param prop Property name.
/// @param value Value to set.
/// @param size Number of bytes to write (valid values are 1, 2, or 4). This value is auto-detected, and the size parameter is only used as a fallback in case detection fails.
/// @param element Element # (starting from 0) if property is an array.
/// @return true if the value was changed, false if an error occurred
bool CEntPropUtils::SetEntProp(int entity, PropType proptype, const char *prop, int value, int size, int element)
{
	edict_t *pEdict;
	CBaseEntity *pEntity;
	SourceMod::sm_sendprop_info_t info;
	SendProp *pProp = nullptr;
	int bit_count;
	int offset;
	bool is_unsigned = false;

	if (!IndexToAThings(entity, &pEntity, &pEdict))
	{
		return false;
	}

	switch (proptype)
	{
	case Prop_Data:
		typedescription_t* td;
		SourceMod::sm_datatable_info_t dinfo;

		if (!FindDataMap(pEntity, dinfo, prop))
		{
			return false;
		}

		td = dinfo.prop;

		if ((bit_count = MatchTypeDescAsInteger(td->fieldType, td->flags)) == 0)
		{
			return false;
		}

		CHECK_SET_PROP_DATA_OFFSET(false);
		
		if (td->fieldType == FIELD_CUSTOM && (td->flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT)
		{
			auto *pVariant = (variant_t *)((intptr_t)pEntity + offset);
			// These are the only three int-ish types that variants support. If set to valid one that isn't
			// (32-bit) integer, leave it alone. It's probably the intended type.
			if (pVariant->fieldType != FIELD_COLOR32 && pVariant->fieldType != FIELD_BOOLEAN)
			{
				pVariant->fieldType = FIELD_INTEGER;
			}

			bit_count = MatchTypeDescAsInteger(pVariant->fieldType, 0);
		}

		SET_TYPE_IF_VARIANT(FIELD_INTEGER);

		break;

	case Prop_Send:
		
		if (!FindSendProp(&info, pEntity, prop))
		{
			return false;
		}

		offset = info.actual_offset;
		pProp = info.prop;
		bit_count = pProp->m_nBits;

		PROP_TYPE_SWITCH(DPT_Int, "integer", false);

		#if SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS \
			|| SOURCE_ENGINE == SE_BMS || SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_TF2 \
			|| SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE || SOURCE_ENGINE == SE_PVKII
			if (pProp->GetFlags() & SPROP_VARINT)
			{
				bit_count = sizeof(int) * 8;
			}
		#endif

		is_unsigned = ((pProp->GetFlags() & SPROP_UNSIGNED) == SPROP_UNSIGNED);

		break;
	
	default:
		return false;
		break;
	}

	if (bit_count < 1)
	{
		bit_count = size * 8;
	}

	if (bit_count >= 17)
	{
		*(int32_t *)((uint8_t *)pEntity + offset) = value;
	}
	else if (bit_count >= 9)
	{
		*(int16_t *)((uint8_t *)pEntity + offset) = (int16_t)value;
	}
	else if (bit_count >= 2)
	{
		*(int8_t *)((uint8_t *)pEntity + offset) = (int8_t)value;
	}
	else
	{
		*(bool *)((uint8_t *)pEntity + offset) = value ? true : false;
	}
	
	if (proptype == Prop_Send && (pEdict != NULL))
	{
		gamehelpers->SetEdictStateChanged(pEdict, offset);
	}

	return true;
}

/**
 * @brief Gets a float value in an entity's property.
 * @param entity Entity/edict index.
 * @param proptype Property type.
 * @param prop Property name.
 * @param result The lookup result will be stored here
 * @param element Element # (starting from 0) if property is an array.
 * @return TRUE on success, FALSE on failure
*/
bool CEntPropUtils::GetEntPropFloat(int entity, PropType proptype, const char* prop, float& result, int element)
{
	CBaseEntity* pEntity = gamehelpers->ReferenceToEntity(entity);
	return GetEntPropFloat(pEntity, proptype, prop, result, element);
}

bool CEntPropUtils::GetEntPropFloat(edict_t* entity, PropType proptype, const char* prop, float& result, int element)
{
	CBaseEntity* pEntity = UtilHelpers::EdictToBaseEntity(entity);
	return GetEntPropFloat(pEntity, proptype, prop, result, element);
}

bool CEntPropUtils::GetEntPropFloat(CBaseEntity* pEntity, PropType proptype, const char* prop, float& result, int element)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CEntPropUtils::GetEntPropFloat", "NavBot");
#endif // EXT_VPROF_ENABLED

	SourceMod::sm_sendprop_info_t info;
	SendProp* pProp = nullptr;
	int bit_count;
	int offset;

	switch (proptype)
	{
	case Prop_Data:
		typedescription_t* td;
		SourceMod::sm_datatable_info_t dinfo;

		if (!FindDataMap(pEntity, dinfo, prop))
		{
			return false;
		}

		td = dinfo.prop;

		CHECK_SET_PROP_DATA_OFFSET(false);

		CHECK_TYPE_VALID_IF_VARIANT(FIELD_FLOAT, "float", false);

		break;

	case Prop_Send:

		if (!FindSendProp(&info, pEntity, prop))
		{
			return false;
		}

		offset = info.actual_offset;
		pProp = info.prop;
		bit_count = pProp->m_nBits;

		PROP_TYPE_SWITCH(DPT_Float, "float", false);

		break;

	default:
		return false;
		break;
	}

	result = *(float*)((uint8_t*)pEntity + offset);
	return true;
}

/// @brief Sets a float value in an entity's property.
/// @param entity Entity/edict index.
/// @param proptype Property type.
/// @param prop Property name.
/// @param value Value to set.
/// @param element Element # (starting from 0) if property is an array.
/// @return true if the value was changed, false if an error occurred
bool CEntPropUtils::SetEntPropFloat(int entity, PropType proptype, const char *prop, float value, int element)
{
	edict_t *pEdict;
	CBaseEntity *pEntity;
	SourceMod::sm_sendprop_info_t info;
	SendProp *pProp = nullptr;
	int bit_count;
	int offset;

	if (!IndexToAThings(entity, &pEntity, &pEdict))
	{
		return false;
	}

	switch (proptype)
	{
	case Prop_Data:
		typedescription_t* td;
		SourceMod::sm_datatable_info_t dinfo;

		if (!FindDataMap(pEntity, dinfo, prop))
		{
			return false;
		}

		td = dinfo.prop;

		if (td->fieldType != FIELD_FLOAT
			&& td->fieldType != FIELD_TIME
			&& (td->fieldType != FIELD_CUSTOM || (td->flags & FTYPEDESC_OUTPUT) != FTYPEDESC_OUTPUT))
		{
			return false;
		}

		CHECK_SET_PROP_DATA_OFFSET(false);

		SET_TYPE_IF_VARIANT(FIELD_FLOAT);

		break;

	case Prop_Send:
		
		if (!FindSendProp(&info, pEntity, prop))
		{
			return false;
		}

		offset = info.actual_offset;
		pProp = info.prop;
		bit_count = pProp->m_nBits;

		PROP_TYPE_SWITCH(DPT_Float, "float", false);

		break;
	
	default:
		return 0.0f;
		break;
	}

	*(float *)((uint8_t *)pEntity + offset) = value;

	if (proptype == Prop_Send && (pEdict != NULL))
	{
		gamehelpers->SetEdictStateChanged(pEdict, offset);
	}

	return true;
}

/**
 * @brief Retrieves an entity index from an entity's property.
 * @param entity Entity/edict index.
 * @param proptype Property type.
 * @param prop Property name.
 * @param result The lookup result will be stored here.
 * @param element Element # (starting from 0) if property is an array.
 * @return TRUE on success, FALSE on failure
*/
bool CEntPropUtils::GetEntPropEnt(int entity, PropType proptype, const char* prop, int* result, CBaseEntity** pOut, int element)
{
	CBaseEntity* pEntity = gamehelpers->ReferenceToEntity(entity);
	return GetEntPropEnt(pEntity, proptype, prop, result, pOut, element);
}

bool CEntPropUtils::GetEntPropEnt(edict_t* entity, PropType proptype, const char* prop, int* result, CBaseEntity** pOut, int element)
{
	CBaseEntity* pEntity = UtilHelpers::EdictToBaseEntity(entity);
	return GetEntPropEnt(pEntity, proptype, prop, result, pOut, element);
}

bool CEntPropUtils::GetEntPropEnt(CBaseEntity* pEntity, PropType proptype, const char* prop, int* result, CBaseEntity** pOut, int element)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CEntPropUtils::GetEntPropEnt", "NavBot");
#endif // EXT_VPROF_ENABLED

	// edict_t* pEdict = UtilHelpers::BaseEntityToEdict(pEntity);
	SourceMod::sm_sendprop_info_t info;
	SendProp* pProp = nullptr;
	int bit_count;
	int offset;
	PropEntType type = PropEnt_Unknown;

	switch (proptype)
	{
	case Prop_Data:
		typedescription_t* td;
		SourceMod::sm_datatable_info_t dinfo;

		if (!FindDataMap(pEntity, dinfo, prop))
		{
			return false;
		}

		td = dinfo.prop;

		switch (td->fieldType)
		{
		case FIELD_EHANDLE:
			type = PropEnt_Handle;
			break;
		case FIELD_CLASSPTR:
			type = PropEnt_Entity;
			break;
		case FIELD_EDICT:
			type = PropEnt_Edict;
			break;
		case FIELD_CUSTOM:
			if ((td->flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT)
			{
				type = PropEnt_Variant;
			}
			break;
		}

		if (type == PropEnt_Unknown)
		{
			return false;
		}

		CHECK_SET_PROP_DATA_OFFSET(false);

		CHECK_TYPE_VALID_IF_VARIANT(FIELD_EHANDLE, "ehandle", false);

		break;

	case Prop_Send:

		type = PropEnt_Handle;

		if (!FindSendProp(&info, pEntity, prop))
		{
			return false;
		}

		offset = info.actual_offset;
		pProp = info.prop;
		bit_count = pProp->m_nBits;

		PROP_TYPE_SWITCH(DPT_Int, "integer", false);

		break;

	default:
		return false;
		break;
	}

	switch (type)
	{
	case PropEnt_Handle:
	case PropEnt_Variant:
	{
		CBaseHandle* hndl;
		if (type == PropEnt_Handle)
		{
			hndl = (CBaseHandle*)((uint8_t*)pEntity + offset);
		}
		else // PropEnt_Variant
		{
			auto* pVariant = (variant_t*)((intptr_t)pEntity + offset);
			hndl = &pVariant->eVal;
		}

		CBaseEntity* pHandleEntity = gamehelpers->ReferenceToEntity(hndl->GetEntryIndex());

		if (!pHandleEntity || *hndl != reinterpret_cast<IHandleEntity*>(pHandleEntity)->GetRefEHandle())
			return false;

		if (result)
		{
			*result = gamehelpers->EntityToBCompatRef(pHandleEntity);
		}

		if (pOut)
		{
			*pOut = pHandleEntity;
		}

		return true;
	}
	case PropEnt_Entity:
	{
		CBaseEntity* pPropEntity = *(CBaseEntity**)((uint8_t*)pEntity + offset);

		if (result)
		{
			*result = gamehelpers->EntityToBCompatRef(pPropEntity);
		}

		if (pOut)
		{
			*pOut = pPropEntity;
		}

		return true;
	}
	case PropEnt_Edict:
	{
		edict_t* _pEdict = *(edict_t**)((uint8_t*)pEntity + offset);
		if (!_pEdict || _pEdict->IsFree()) {
			return false;
		}

		if (result)
		{
			*result = gamehelpers->IndexOfEdict(_pEdict);
		}

		if (pOut)
		{
			*pOut = UtilHelpers::EdictToBaseEntity(_pEdict);
		}

		return true;
	}
	}

	return false;
}

/// @brief Sets an entity index in an entity's property.
/// @param entity Entity/edict index.
/// @param proptype Property type.
/// @param prop Property name.
/// @param other Entity index to set, or -1 to unset.
/// @param element Element # (starting from 0) if property is an array.
/// @return true if the value was changed, false if an error occurred
bool CEntPropUtils::SetEntPropEnt(int entity, PropType proptype, const char *prop, int other, int element)
{
	edict_t *pEdict;
	CBaseEntity *pEntity;
	SourceMod::sm_sendprop_info_t info;
	SendProp *pProp = nullptr;
	int bit_count;
	int offset;
	PropEntType type = PropEnt_Unknown;

	if (!IndexToAThings(entity, &pEntity, &pEdict))
	{
		return false;
	}

	switch (proptype)
	{
	case Prop_Data:
		typedescription_t* td;
		SourceMod::sm_datatable_info_t dinfo;

		if (!FindDataMap(pEntity, dinfo, prop))
		{
			return false;
		}

		td = dinfo.prop;

		switch (td->fieldType)
		{
		case FIELD_EHANDLE:
			type = PropEnt_Handle;
			break;
		case FIELD_CLASSPTR:
			type = PropEnt_Entity;
			break;
		case FIELD_EDICT:
			type = PropEnt_Edict;
			break;
		case FIELD_CUSTOM:
			if ((td->flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT)
			{
				type = PropEnt_Variant;
			}
			break;
		}

		if (type == PropEnt_Unknown)
		{
			return false;
		}

		CHECK_SET_PROP_DATA_OFFSET(false);

		CHECK_TYPE_VALID_IF_VARIANT(FIELD_EHANDLE, "ehandle", false);

		break;

	case Prop_Send:
		
		type = PropEnt_Handle;

		if (!FindSendProp(&info, pEntity, prop))
		{
			return false;
		}

		offset = info.actual_offset;
		pProp = info.prop;
		bit_count = pProp->m_nBits;

		PROP_TYPE_SWITCH(DPT_Int, "integer", false);

		break;
	
	default:
		return false;
		break;
	}

	CBaseEntity *pOther = GetEntity(other);
	if (!pOther && other != -1)
	{
		return false;
	}

	switch (type)
	{
	case PropEnt_Handle:
	case PropEnt_Variant:
		{
			CBaseHandle *hndl;
			if (type == PropEnt_Handle)
			{
				hndl = (CBaseHandle *)((uint8_t *)pEntity + offset);
			}
			else // PropEnt_Variant
			{
				auto *pVariant = (variant_t *)((intptr_t)pEntity + offset);
				hndl = &pVariant->eVal;
			}

			hndl->Set((IHandleEntity *) pOther);

			if (proptype == Prop_Send && (pEdict != NULL))
			{
				gamehelpers->SetEdictStateChanged(pEdict, offset);
			}
		}

		break;

	case PropEnt_Entity:
		{
			*(CBaseEntity **) ((uint8_t *) pEntity + offset) = pOther;
			break;
		}

	case PropEnt_Edict:
		{
			edict_t *pOtherEdict = NULL;
			if (pOther)
			{
				IServerNetworkable *pNetworkable = ((IServerUnknown *) pOther)->GetNetworkable();
				if (!pNetworkable)
				{
					return false;
				}

				pOtherEdict = pNetworkable->GetEdict();
				if (!pOtherEdict || pOtherEdict->IsFree())
				{
					return false;
				}
			}

			*(edict_t **) ((uint8_t *) pEntity + offset) = pOtherEdict;
			break;
		}
	}

	return true;
}

/// @brief Retrieves a vector of floats from an entity, given a named network property.
/// @param entity Entity/edict index.
/// @param proptype Property type.
/// @param prop Property name.
/// @param element Element # (starting from 0) if property is an array.
/// @return Value at the given property offset.
bool CEntPropUtils::GetEntPropVector(int entity, PropType proptype, const char* prop, Vector& result, int element)
{
	CBaseEntity* pEntity = gamehelpers->ReferenceToEntity(entity);
	return GetEntPropVector(pEntity, proptype, prop, result, element);
}

bool CEntPropUtils::GetEntPropVector(edict_t* entity, PropType proptype, const char* prop, Vector& result, int element)
{
	CBaseEntity* pEntity = UtilHelpers::EdictToBaseEntity(entity);
	return GetEntPropVector(pEntity, proptype, prop, result, element);
}

bool CEntPropUtils::GetEntPropVector(CBaseEntity* pEntity, PropType proptype, const char* prop, Vector& result, int element)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CEntPropUtils::GetEntPropVector", "NavBot");
#endif // EXT_VPROF_ENABLED

	SourceMod::sm_sendprop_info_t info;
	SendProp* pProp = nullptr;
	int bit_count;
	int offset;
	bool is_unsigned = false;

	switch (proptype)
	{
	case Prop_Data:
		typedescription_t* td;
		SourceMod::sm_datatable_info_t dinfo;

		if (!FindDataMap(pEntity, dinfo, prop))
		{
			return false;
		}

		td = dinfo.prop;

		if (td->fieldType != FIELD_VECTOR && td->fieldType != FIELD_POSITION_VECTOR)
		{
			return false;
		}

		CHECK_SET_PROP_DATA_OFFSET(false);

		if (td->fieldType == FIELD_CUSTOM && (td->flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT)
		{
			variant_t* pVariant = (variant_t*)((intptr_t)pEntity + offset);

			if (pVariant->fieldType != FIELD_VECTOR && pVariant->fieldType != FIELD_POSITION_VECTOR)
			{
				return false;
			}
		}

		break;

	case Prop_Send:

		if (!FindSendProp(&info, pEntity, prop))
		{
			return false;
		}

		offset = info.actual_offset;
		pProp = info.prop;
		bit_count = pProp->m_nBits;

		PROP_TYPE_SWITCH(DPT_Vector, "vector", false);

		break;

	default:
		return false;
		break;
	}

	Vector* v = (Vector*)((uint8_t*)pEntity + offset);
	result = *v;

	return true;
}

/// @brief Sets a vector of floats in an entity, given a named network property.
/// @param entity Entity/edict index.
/// @param proptype Property type.
/// @param prop Property name.
/// @param value Vector to set.
/// @param element Element # (starting from 0) if property is an array.
/// @return true if the value was changed, false if an error occurred
bool CEntPropUtils::SetEntPropVector(int entity, PropType proptype, const char *prop, Vector value, int element)
{
	edict_t *pEdict;
	CBaseEntity *pEntity;
	SourceMod::sm_sendprop_info_t info;
	SendProp *pProp = nullptr;
	int bit_count;
	int offset;
	bool is_unsigned = false;

	if (!IndexToAThings(entity, &pEntity, &pEdict))
	{
		return false;
	}

	switch (proptype)
	{
	case Prop_Data:
		typedescription_t* td;
		SourceMod::sm_datatable_info_t dinfo;

		if (!FindDataMap(pEntity, dinfo, prop))
		{
			return false;
		}

		td = dinfo.prop;

		if (td->fieldType != FIELD_VECTOR && td->fieldType != FIELD_POSITION_VECTOR)
		{
			return false;
		}

		CHECK_SET_PROP_DATA_OFFSET(false);
		
		if (td->fieldType == FIELD_CUSTOM && (td->flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT)
		{
			auto *pVariant = (variant_t *)((intptr_t)pEntity + offset);
			// Both of these are supported and we don't know which is intended. But, if it's already
			// a pos vector, we probably want to keep that.
			if (pVariant->fieldType != FIELD_POSITION_VECTOR)
			{
				pVariant->fieldType = FIELD_VECTOR;
			}
		}

		break;

	case Prop_Send:
		
		if (!FindSendProp(&info, pEntity, prop))
		{
			return false;
		}

		offset = info.actual_offset;
		pProp = info.prop;
		bit_count = pProp->m_nBits;

		PROP_TYPE_SWITCH(DPT_Vector, "vector", false);

		break;
	
	default:
		return false;
		break;
	}

	Vector *v = (Vector *)((uint8_t *)pEntity + offset);

	*v = value;

	if (proptype == Prop_Send && (pEdict != NULL))
	{
		gamehelpers->SetEdictStateChanged(pEdict, offset);
	}

	return true;
}

/// @brief Gets a network property as a string.
/// @param entity Edict index.
/// @param proptype Property type.
/// @param prop Property to use.
/// @param maxlen
/// @param len 
/// @param element Element # (starting from 0) if property is an array.
/// @return Value at the given property offset.
bool CEntPropUtils::GetEntPropString(int entity, PropType proptype, const char* prop, char* result, int maxlen, size_t& len, int element)
{
	CBaseEntity* pEntity = gamehelpers->ReferenceToEntity(entity);
	return GetEntPropString(pEntity, proptype, prop, result, maxlen, len, element);
}

bool CEntPropUtils::GetEntPropString(edict_t* entity, PropType proptype, const char* prop, char* result, int maxlen, size_t& len, int element)
{
	CBaseEntity* pEntity = UtilHelpers::EdictToBaseEntity(entity);
	return GetEntPropString(pEntity, proptype, prop, result, maxlen, len, element);
}

bool CEntPropUtils::GetEntPropString(CBaseEntity* pEntity, PropType proptype, const char* prop, char* result, int maxlen, size_t& len, int element)
{
#ifdef EXT_VPROF_ENABLED
	VPROF_BUDGET("CEntPropUtils::GetEntPropString", "NavBot");
#endif // EXT_VPROF_ENABLED

	SourceMod::sm_sendprop_info_t info;
	SendProp* pProp = nullptr;
	int bit_count;
	int offset;
	const char* src = nullptr;
	bool bIsStringIndex = false;

	switch (proptype)
	{
	case Prop_Data:
		typedescription_t* td;
		SourceMod::sm_datatable_info_t dinfo;

		if (!FindDataMap(pEntity, dinfo, prop))
		{
			return false;
		}

		td = dinfo.prop;

		if ((td->fieldType != FIELD_CHARACTER
			&& td->fieldType != FIELD_STRING
			&& td->fieldType != FIELD_MODELNAME
			&& td->fieldType != FIELD_SOUNDNAME)
			|| (td->fieldType == FIELD_CUSTOM && (td->flags & FTYPEDESC_OUTPUT) != FTYPEDESC_OUTPUT))
		{
			return false;
		}

		bIsStringIndex = (td->fieldType != FIELD_CHARACTER);

		if (element != 0)
		{
			if (bIsStringIndex)
			{
				if (element < 0 || element >= td->fieldSize)
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}

		offset = dinfo.actual_offset;

		if (bIsStringIndex)
		{
			offset += (element * (td->fieldSizeInBytes / td->fieldSize));

			string_t idx;

			idx = *(string_t*)((uint8_t*)pEntity + offset);
			src = (idx == NULL_STRING) ? "" : STRING(idx);
		}
		else
		{
			src = (char*)((uint8_t*)pEntity + offset);
		}

		break;

	case Prop_Send:

		if (!FindSendProp(&info, pEntity, prop))
		{
			return false;
		}

		offset = info.actual_offset;
		pProp = info.prop;
		bit_count = pProp->m_nBits;

		PROP_TYPE_SWITCH(DPT_String, "string", false);

		if (pProp->GetProxyFn())
		{
			DVariant var;
			int entindex = UtilHelpers::IndexOfEntity(pEntity);
			pProp->GetProxyFn()(pProp, pEntity, (const void*)((intptr_t)pEntity + offset), &var, element, entindex);
			src = (char*)var.m_pString; // hack because SDK 2013 declares this as const char*
		}
		else
		{
			src = *(char**)((uint8_t*)pEntity + offset);
		}

		break;

	default:
		return false;
		break;
	}

	size_t length = ke::SafeStrcpy(result, maxlen, src);
	len = length;

	return true;
}


/// @brief Sets a network property as a string.
/// @warning This is not implemented yet!
bool CEntPropUtils::SetEntPropString(int entity, PropType proptype, const char *prop, char *value, int element)
{
	return false;

#if 0 // Not supported for now

	edict_t *pEdict;
	CBaseEntity *pEntity;
	SourceMod::sm_sendprop_info_t info;
	SendProp *pProp = nullptr;
	int bit_count;
	int offset;
	int maxlen;
	bool bIsStringIndex = false;

	if (!IndexToAThings(entity, &pEntity, &pEdict))
	{
		// TO-DO: CHANGE ME!! logger->Log(LogLevel::ERROR, "Entity %d (%d) is invalid", gamehelpers->ReferenceToIndex(entity), entity);
		return false;
	}

	switch (proptype)
	{
	case Prop_Data:
		typedescription_t *td;
		datamap_t *pMap;

		if ((pMap = gamehelpers->GetDataMap(pEntity)) == NULL)
		{
			// TO-DO: CHANGE ME!! logger->Log(LogLevel::ERROR, "Could not retrieve datamap for %s", pEdict->GetClassName());
			return false;
		}

		SourceMod::sm_datatable_info_t dinfo;

		if (!gamehelpers->FindDataMapInfo(pMap, prop, &dinfo))
		{
			const char *classname = gamehelpers->GetEntityClassname(pEntity);
			// TO-DO: CHANGE ME!! logger->Log(LogLevel::ERROR, "Property \"%s\" not found (entity %d/%s)", prop, entity, (classname ? classname : ""));
			return false;
		}

		td = dinfo.prop;

		if ((td->fieldType != FIELD_CHARACTER
			&& td->fieldType != FIELD_STRING
			&& td->fieldType != FIELD_MODELNAME 
			&& td->fieldType != FIELD_SOUNDNAME)
			|| (td->fieldType == FIELD_CUSTOM && (td->flags & FTYPEDESC_OUTPUT) != FTYPEDESC_OUTPUT))
		{
			// TO-DO: CHANGE ME!! logger->Log(LogLevel::ERROR, "Data field %s is not a string (%d != %d)", prop, td->fieldType, FIELD_CHARACTER);
			return false;
		}

		bIsStringIndex = (td->fieldType != FIELD_CHARACTER);

		if (element != 0)
		{
			if (bIsStringIndex)
			{
				if (element < 0 || element >= td->fieldSize)
				{
					// TO-DO: CHANGE ME!! logger->Log(LogLevel::ERROR, "Element %d is out of bounds (Prop %s has %d elements).", element, prop, td->fieldSize);
					return false;
				}
			}
			else
			{
				// TO-DO: CHANGE ME!! logger->Log(LogLevel::ERROR, "Prop %s is not an array. Element %d is invalid.", prop, element);
				return false;
			}
		}

		offset = dinfo.actual_offset;

		if (bIsStringIndex)
		{
			offset += (element * (td->fieldSizeInBytes / td->fieldSize));
		}
		else
		{
			maxlen = td->fieldSize;
		}

		SET_TYPE_IF_VARIANT(FIELD_STRING);

		break;

	case Prop_Send:
		
		if (!FindSendProp(&info, pEntity, prop, entity))
		{
			// TO-DO: CHANGE ME!! logger->Log(LogLevel::ERROR, "Failed to look up \"%s\" property.", prop);
			return false;
		}

		offset = info.actual_offset;
		pProp = info.prop;
		bit_count = pProp->m_nBits;

		PROP_TYPE_SWITCH(DPT_String, "string", false);

		bIsStringIndex = false;
		if (pProp->GetProxyFn())
		{
			DVariant var;
			pProp->GetProxyFn()(pProp, pEntity, (const void *) ((intptr_t) pEntity + offset), &var, element, entity);
			if (var.m_pString == ((string_t *) ((intptr_t) pEntity + offset))->ToCStr())
			{
				bIsStringIndex = true;
			}
		}

		// Only used if not string index.
		maxlen = DT_MAX_STRING_BUFFERSIZE;

		break;
	
	default:
		// TO-DO: CHANGE ME!! logger->Log(LogLevel::ERROR, "Invalid PropType %d", proptype);
		return false;
		break;
	}

	size_t len;

	if (bIsStringIndex)
	{
		*(string_t *) ((intptr_t) pEntity + offset) = g_HL2.AllocPooledString(value);
		len = strlen(value);
	}
	else
	{
		char *dest = (char *) ((uint8_t *) pEntity + offset);
		len = ke::SafeStrcpy(dest, maxlen, value);
	}

	if (proptype == Prop_Send && (pEdict != NULL))
	{
		g_HL2.SetEdictStateChanged(pEdict, offset);
	}

	return true;

#endif
}

/// @brief Peeks into an entity's object data and retrieves the integer value at the given offset.
/// @param entity Edict index.
/// @param offset Offset to use.
/// @param size Number of bytes to read (valid values are 1, 2, or 4).
/// @return Value at the given memory location.
bool CEntPropUtils::GetEntData(int entity, int offset, int& result, int size)
{
	CBaseEntity *pEntity = GetEntity(entity);

	if (!pEntity)
	{
		return false;
	}

	if (offset <= 0 || offset > 32768)
	{
		return false;
	}

	switch (size)
	{
	case 4:
		result = *(int*)((uint8_t*)pEntity + offset);
		return true;
	case 2:
		result = *(short*)((uint8_t*)pEntity + offset);
		return true;
	case 1:
		result = *((uint8_t*)pEntity + offset);
		return true;
	default:
		return false;
	}
}

/// @brief Peeks into an entity's object data and sets the integer value at the given offset.
/// @param entity Edict index.
/// @param offset Offset to use.
/// @param value Value to set.
/// @param size Number of bytes to write (valid values are 1, 2, or 4).
/// @param changeState If true, change will be sent over the network.
/// @return true on success, false on failure
bool CEntPropUtils::SetEntData(int entity, int offset, int value, int size, bool changeState)
{
	CBaseEntity *pEntity;
	edict_t *pEdict;

	if (!IndexToAThings(entity, &pEntity, &pEdict))
	{
		return false;
	}

	if (offset <= 0 || offset > 32768)
	{
		return false;
	}

	if (changeState && (pEdict != NULL))
	{
		gamehelpers->SetEdictStateChanged(pEdict, offset);
	}

	switch (size)
	{
	case 4:
		{
			*(int *)((uint8_t *)pEntity + offset) = value;
			break;
		}
	case 2:
		{
			*(short *)((uint8_t *)pEntity + offset) = value;
			break;
		}
	case 1:
		{
			*((uint8_t *)pEntity + offset) = value;
			break;
		}
	default:
		return false;
	}

	return true;
}

/// @brief Peeks into an entity's object data and retrieves the float value at the given offset.
/// @param entity Edict index.
/// @param offset Offset to use.
/// @return Value at the given memory location.
bool CEntPropUtils::GetEntDataFloat(int entity, int offset, float& result)
{
	CBaseEntity *pEntity = GetEntity(entity);

	if (!pEntity)
	{
		return false;
	}

	if (offset <= 0 || offset > 32768)
	{
		return false;
	}

	result = *(float*)((uint8_t*)pEntity + offset);
	return true;
}

/// @brief Peeks into an entity's object data and sets the float value at the given offset.
/// @param entity Edict index.
/// @param offset Offset to use.
/// @param value Value to set.
/// @param changeState If true, change will be sent over the network.
/// @return true on success, false on failure
bool CEntPropUtils::SetEntDataFloat(int entity, int offset, float value, bool changeState)
{
	CBaseEntity *pEntity;
	edict_t *pEdict;

	if (!IndexToAThings(entity, &pEntity, &pEdict))
	{
		return false;
	}

	if (offset <= 0 || offset > 32768)
	{
		return false;
	}

	*(float *)((uint8_t *)pEntity + offset) = value;

	if (changeState && (pEdict != NULL))
	{
		gamehelpers->SetEdictStateChanged(pEdict, offset);
	}

	return true;
}

/// @brief Peeks into an entity's object data and retrieves the entity index at the given offset.
/// Note: This will only work on offsets that are stored as "entity
/// handles" (which usually looks like m_h* in properties).  These
/// are not SourceMod Handles, but internal Source structures.
/// @param entity Edict index.
/// @param offset Offset to use.
/// @return Entity index at the given location. If there is no entity, or the stored entity is invalid, then -1 is returned.
bool CEntPropUtils::GetEntDataEnt(int entity, int offset, int& result)
{
	CBaseEntity *pEntity = GetEntity(entity);

	if (!pEntity)
	{
		return false;
	}

	if (offset <= 0 || offset > 32768)
	{
		return false;
	}

	CBaseHandle &hndl = *(CBaseHandle *)((uint8_t *)pEntity + offset);
	CBaseEntity *pHandleEntity = gamehelpers->ReferenceToEntity(hndl.GetEntryIndex());

	if (!pHandleEntity || hndl != reinterpret_cast<IHandleEntity *>(pHandleEntity)->GetRefEHandle())
		return false;

	result = gamehelpers->EntityToBCompatRef(pHandleEntity);
	return true;
}

/// @brief Peeks into an entity's object data and sets the entity index at the given offset.
/// Note: This will only work on offsets that are stored as "entity
/// handles" (which usually looks like m_h* in properties).  These
/// are not SourceMod Handles, but internal Source structures.
/// @param entity Edict index.
/// @param offset Offset to use.
/// @param value Entity index to set, or -1 to clear.
/// @param changeState If true, change will be sent over the network.
/// @return true on success, false on failure
bool CEntPropUtils::SetEntDataEnt(int entity, int offset, int value, bool changeState)
{
	CBaseEntity *pEntity;
	edict_t *pEdict;

	if (!IndexToAThings(entity, &pEntity, &pEdict))
	{
		return false;
	}

	if (offset <= 0 || offset > 32768)
	{
		return false;
	}

	CBaseHandle &hndl = *(CBaseHandle *)((uint8_t *)pEntity + offset);

	if ((unsigned)value == INVALID_EHANDLE_INDEX)
	{
		hndl.Set(NULL);
	}
	else
	{
		CBaseEntity *pOther = GetEntity(value);

		if (!pOther)
		{
			return false;
		}

		IHandleEntity *pHandleEnt = (IHandleEntity *)pOther;
		hndl.Set(pHandleEnt);
	}

	if (changeState && (pEdict != NULL))
	{
		gamehelpers->SetEdictStateChanged(pEdict, offset);
	}

	return true;
}

/// @brief Peeks into an entity's object data and retrieves the vector at the given offset.
/// @param entity Edict index.
/// @param offset Offset to use.
/// @return Vector value at the given memory location.
bool CEntPropUtils::GetEntDataVector(int entity, int offset, Vector& result)
{
	CBaseEntity *pEntity = GetEntity(entity);

	if (!pEntity)
	{
		return false;
	}

	if (offset <= 0 || offset > 32768)
	{
		return false;
	}

	Vector *v = (Vector *)((uint8_t *)pEntity + offset);
	result = *v;
	return true;
}

/// @brief Peeks into an entity's object data and sets the vector at the given offset.
/// @param entity Edict index.
/// @param offset Offset to use.
/// @param value Vector to set.
/// @param changeState If true, change will be sent over the network.
/// @return true on success, false on failure
bool CEntPropUtils::SetEntDataVector(int entity, int offset, Vector value, bool changeState)
{
	CBaseEntity *pEntity;
	edict_t *pEdict;

	if (!IndexToAThings(entity, &pEntity, &pEdict))
	{
		return false;
	}

	if (offset <= 0 || offset > 32768)
	{
		return false;
	}

	Vector *v = (Vector *)((uint8_t *)pEntity + offset);

	*v = value;

	if (changeState && (pEdict != NULL))
	{
		gamehelpers->SetEdictStateChanged(pEdict, offset);
	}

	return true;
}

/// @brief Peeks into an entity's object data and retrieves the string at the given offset.
/// @param entity Edict index.
/// @param offset Offset to use.
/// @param maxlen Maximum length of output string buffer.
/// @param len Number of non-null bytes written.
/// @return String pointer at the given memory location.
bool CEntPropUtils::GetEntDataString(int entity, int offset, int maxlen, char* result, size_t& len)
{
	CBaseEntity *pEntity = GetEntity(entity);

	if (!pEntity)
	{
		return false;
	}

	if (offset <= 0 || offset > 32768)
	{
		return false;
	}

	if (maxlen <= 0)
	{
		return false;
	}

	char *src = (char *)((uint8_t *)pEntity + offset);
	size_t length = ke::SafeStrcpy(result, maxlen, src);
	len = length;
	return true;
}

/// @brief 
/// @param entity Edict index.
/// @param offset Offset to use.
/// @param value String to set.
/// @param maxlen Maximum length of output string buffer.
/// @param changeState If true, change will be sent over the network.
/// @return true on success, false on failure
bool CEntPropUtils::SetEntDataString(int entity, int offset, char *value, int maxlen, bool changeState)
{
	CBaseEntity *pEntity;
	edict_t *pEdict;

	if (!IndexToAThings(entity, &pEntity, &pEdict))
	{
		return false;
	}

	if (offset <= 0 || offset > 32768)
	{
		return false;
	}

	char *src = nullptr;
	char *dest = (char *)((uint8_t *)pEntity + offset);

	ke::SafeStrcpy(dest, maxlen, src);

	if (changeState && (pEdict != NULL))
	{
		gamehelpers->SetEdictStateChanged(pEdict, offset);
	}

	return true;
}

std::size_t CEntPropUtils::GetEntPropArraySize(int entity, PropType proptype, const char* prop)
{
	CBaseEntity* pEntity = nullptr;

	if (!IndexToAThings(entity, &pEntity, nullptr))
	{
		return 0;
	}

	switch (proptype)
	{
	case Prop_Data:
	{
		typedescription_t* td = nullptr;
		SourceMod::sm_datatable_info_t dinfo;

		if (!FindDataMap(pEntity, dinfo, prop))
		{
			return 0;
		}

		td = dinfo.prop;

		return static_cast<std::size_t>(td->fieldSize);
	}
	case Prop_Send:
	{
		SourceMod::sm_sendprop_info_t info;

		if (!FindSendProp(&info, pEntity, prop))
		{
			return 0;
		}

		if (info.prop->GetType() == SendPropType::DPT_Array)
		{
			return static_cast<std::size_t>(info.prop->GetNumElements());
		}

		if (info.prop->GetType() != SendPropType::DPT_DataTable)
		{
			return 0;
		}

		SendTable* pTable = info.prop->GetDataTable();

		if (!pTable)
		{
			return 0;
		}

		return static_cast<std::size_t>(pTable->GetNumProps());
	}
	default:
		return 0;
	}

	return 0;
}

CBaseEntity *CEntPropUtils::GetGameRulesProxyEntity()
{
	static int proxyEntRef = -1;
	CBaseEntity *pProxy = nullptr;
	if (proxyEntRef == -1 || (pProxy = gamehelpers->ReferenceToEntity(proxyEntRef)) == NULL)
	{
		pProxy = GetEntity(UtilHelpers::FindEntityByNetClass(playerhelpers->GetMaxClients(), grclassname.c_str()));
		if (pProxy)
			proxyEntRef = gamehelpers->EntityToReference(pProxy);
	}
	
	return pProxy;
}

/// @brief Retrieves an integer value from a property of the gamerules entity.
/// @param prop Property name.
/// @param size Number of bytes to read (valid values are 1, 2, or 4). This value is auto-detected, and the size parameter is only used as a fallback in case detection fails.
/// @param element Element # (starting from 0) if property is an array.
/// @return Value at the given property offset.
bool CEntPropUtils::GameRules_GetProp(const char *prop, int& result, int size, int element)
{
	int offset;
	int bit_count;
	bool is_unsigned = false;
	void *pGameRules = g_pSDKTools->GetGameRules();

	if (!pGameRules || grclassname.empty())
	{
		return false;
	}

	int elementCount = 1;
	GAMERULES_FIND_PROP_SEND(DPT_Int, "integer", false);
	is_unsigned = ((pProp->GetFlags() & SPROP_UNSIGNED) == SPROP_UNSIGNED);

	// This isn't in CS:S yet, but will be, doesn't hurt to add now, and will save us a build later
#if SOURCE_ENGINE == SE_CSS || SOURCE_ENGINE == SE_HL2DM || SOURCE_ENGINE == SE_DODS || SOURCE_ENGINE == SE_TF2 \
	|| SOURCE_ENGINE == SE_SDK2013 || SOURCE_ENGINE == SE_BMS || SOURCE_ENGINE == SE_CSGO || SOURCE_ENGINE == SE_BLADE || SOURCE_ENGINE == SE_PVKII
	if (pProp->GetFlags() & SPROP_VARINT)
	{
		bit_count = sizeof(int) * 8;
	}
#endif

	if (bit_count < 1)
	{
		bit_count = size * 8;
	}

	if (bit_count >= 17)
	{
		result = *(int32_t*)((intptr_t)pGameRules + offset);
		return true;
	}
	else if (bit_count >= 9)
	{
		if (is_unsigned)
		{
			result = *(uint16_t*)((intptr_t)pGameRules + offset);
			return true;
		}
		else
		{
			result = *(int16_t*)((intptr_t)pGameRules + offset);
			return true;
		}
	}
	else if (bit_count >= 2)
	{
		if (is_unsigned)
		{
			result = *(uint8_t*)((intptr_t)pGameRules + offset);
			return true;
		}
		else
		{
			result = *(int8_t*)((intptr_t)pGameRules + offset);
			return true;
		}
	}
	else
	{
		result = *(bool*)((intptr_t)pGameRules + offset) ? 1 : 0;
		return true;
	}

	return false;
}

/// @brief Retrieves a float value from a property of the gamerules entity.
/// @param prop Property name.
/// @param element Element # (starting from 0) if property is an array.
/// @return Value at the given property offset.
bool CEntPropUtils::GameRules_GetPropFloat(const char *prop, float& result, int element)
{
	int offset;
	int bit_count;
	void *pGameRules = g_pSDKTools->GetGameRules();

	if (!pGameRules || grclassname.empty())
	{
		return false;
	}

	int elementCount = 1;
	GAMERULES_FIND_PROP_SEND(DPT_Float, "float", false);

	result = *(float*)((intptr_t)pGameRules + offset);
	return true;
}

/// @brief Retrieves a entity index from a property of the gamerules entity.
/// @param prop Property name.
/// @param element Element # (starting from 0) if property is an array.
/// @return Entity index at the given property. If there is no entity, or the entity is not valid, then -1 is returned.
bool CEntPropUtils::GameRules_GetPropEnt(const char *prop, int& result, int element)
{
	int offset;
	int bit_count;
	void *pGameRules = g_pSDKTools->GetGameRules();

	if (!pGameRules || grclassname.empty())
	{
		return false;
	}

	int elementCount = 1;
	GAMERULES_FIND_PROP_SEND(DPT_Int, "Integer", false);

	CBaseHandle &hndl = *(CBaseHandle *)((intptr_t)pGameRules + offset);
	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(hndl.GetEntryIndex());

	if (!pEntity || ((IServerEntity *)pEntity)->GetRefEHandle() != hndl)
	{
		return false;
	}

	result = gamehelpers->EntityToBCompatRef(pEntity);
	return false;
}

/// @brief Retrieves a vector from the gamerules entity, given a named network property.
/// @param prop Property name.
/// @param element Element # (starting from 0) if property is an array.
/// @return Value at the given property offset.
bool CEntPropUtils::GameRules_GetPropVector(const char *prop, Vector& result, int element)
{
	int offset;
	int bit_count;
	void *pGameRules = g_pSDKTools->GetGameRules();

	if (!pGameRules || grclassname.empty())
	{
		return false;
	}

	int elementCount = 1;
	GAMERULES_FIND_PROP_SEND(DPT_Vector, "vector", false);

	Vector *v = (Vector*)((intptr_t)pGameRules + offset);
	result = *v;
	return true;
}

/// @brief Gets a gamerules property as a string.
/// @param prop Property to use.
/// @param len Number of non-null bytes written.
/// @param maxlen Maximum length of output string buffer.
/// @param element Element # (starting from 0) if property is an array.
/// @return Value at the given property offset.
bool CEntPropUtils::GameRules_GetPropString(const char* prop, char* result, size_t& len, int maxlen, int element)
{
	int offset;
	int bit_count;
	void *pGameRules = g_pSDKTools->GetGameRules();

	if (!pGameRules || grclassname.empty())
	{
		return false;
	}

	int elementCount = 1;
	GAMERULES_FIND_PROP_SEND(DPT_String, "string", false);

	const char *src;
	if (pProp->GetProxyFn())
	{
		DVariant var;
		pProp->GetProxyFn()(pProp, pGameRules, (const void *)((intptr_t)pGameRules + offset), &var, element, 0 /* TODO */);
		src = var.m_pString;
	}
	else
	{
		src = *(char **)((uint8_t *)pGameRules + offset);
	}

	if (src)
	{
		size_t length = ke::SafeStrcpy(result, maxlen, src);
		len = length;
		return true;
	}

	return false;
}

RoundState CEntPropUtils::GameRules_GetRoundState()
{
	int roundstate = 0;
	GameRules_GetProp("m_iRoundState", roundstate);
	return static_cast<RoundState>(roundstate);
}

struct NetPropBuildCacheData
{
	constexpr NetPropBuildCacheData(const char* clname, const char* prop, CEntPropUtils::CacheIndex i) :
		classname(clname), propname(prop), index(i)
	{
	}
	
	std::string_view classname;
	std::string_view propname;
	CEntPropUtils::CacheIndex index;
};

void CEntPropUtils::BuildCache()
{
	/**
	* Build a cache of SendProp (Networked Properties) offsets. This cache is built when the game loads for the first time.
	* This give us a fast way to read these.
	* Important: This is only for networked properties. If you need to cache a datamap, see CEntPropUtils::GetEntityClassname as an example.
	*/

	constexpr std::array properties = {
		NetPropBuildCacheData("CTFPlayer", "m_nPlayerCond", CEntPropUtils::CacheIndex::CTFPLAYER_PLAYERCOND),
		NetPropBuildCacheData("CTFPlayer", "_condition_bits", CEntPropUtils::CacheIndex::CTFPLAYER_PLAYERCONDBITS),
		NetPropBuildCacheData("CTFPlayer", "m_nPlayerCondEx", CEntPropUtils::CacheIndex::CTFPLAYER_PLAYERCONDEX1),
		NetPropBuildCacheData("CTFPlayer", "m_nPlayerCondEx2", CEntPropUtils::CacheIndex::CTFPLAYER_PLAYERCONDEX2),
		NetPropBuildCacheData("CTFPlayer", "m_nPlayerCondEx3", CEntPropUtils::CacheIndex::CTFPLAYER_PLAYERCONDEX3),
		NetPropBuildCacheData("CTFPlayer", "m_nPlayerCondEx4", CEntPropUtils::CacheIndex::CTFPLAYER_PLAYERCONDEX4),
		NetPropBuildCacheData("CTFPlayer", "m_iClass", CEntPropUtils::CacheIndex::CTFPLAYER_CLASSTYPE),
		NetPropBuildCacheData("CBaseCombatCharacter", "m_hActiveWeapon", CEntPropUtils::CacheIndex::CBASECOMBATCHARACTER_ACTIVEWEAPON),
		NetPropBuildCacheData("CBaseCombatCharacter", "m_hMyWeapons", CEntPropUtils::CacheIndex::CBASECOMBATCHARACTER_MYWEAPONS),
		NetPropBuildCacheData("CBaseCombatWeapon", "m_iClip1", CEntPropUtils::CacheIndex::CBASECOMBATWEAPON_CLIP1),
		NetPropBuildCacheData("CBaseCombatWeapon", "m_iClip2", CEntPropUtils::CacheIndex::CBASECOMBATWEAPON_CLIP2),
		NetPropBuildCacheData("CBaseCombatWeapon", "m_iPrimaryAmmoType", CEntPropUtils::CacheIndex::CBASECOMBATWEAPON_PRIMARYAMMOTYPE),
		NetPropBuildCacheData("CBaseCombatWeapon", "m_iSecondaryAmmoType", CEntPropUtils::CacheIndex::CBASECOMBATWEAPON_SECONDARYAMMOTYPE),
		NetPropBuildCacheData("CBaseCombatWeapon", "m_iState", CEntPropUtils::CacheIndex::CBASECOMBATWEAPON_STATE),
		NetPropBuildCacheData("CBaseCombatWeapon", "m_hOwner", CEntPropUtils::CacheIndex::CBASECOMBATWEAPON_OWNER),
		NetPropBuildCacheData("CBasePlayer", "m_nWaterLevel", CEntPropUtils::CacheIndex::CBASEPLAYER_WATERLEVEL),
		NetPropBuildCacheData("CDODPlayer", "m_iPlayerClass", CEntPropUtils::CacheIndex::CDODPLAYER_PLAYERCLASS),
	};

	static_assert(properties.size() == static_cast<size_t>(CacheIndex::CACHEINDEX_SIZE), "CEntPropUtils::BuildCache properties array size mismatch!");

	SourceMod::sm_sendprop_info_t info;

	for (auto& data : properties)
	{
		if (gamehelpers->FindSendPropInfo(data.classname.data(), data.propname.data(), &info))
		{
			cached_offsets[static_cast<int>(data.index)] = info.actual_offset;
		}
	}
}

const char* entityprops::GetEntityClassname(CBaseEntity* entity)
{
	static int s_offset = -1;

	if (s_offset < 0)
	{
		SourceMod::sm_datatable_info_t info;
		if (!gamehelpers->FindDataMapInfo(gamehelpers->GetDataMap(entity), "m_iClassname", &info))
		{
			return nullptr;
		}

		s_offset = static_cast<int>(info.actual_offset);
	}

	string_t s = *(string_t*)((uint8_t*)entity + s_offset);

	if (s == NULL_STRING)
	{
		return nullptr;
	}

	return STRING(s);
}

int entityprops::GetEntityTeamNum(CBaseEntity* entity)
{
	static int s_offset = -1;

	if (s_offset < 0)
	{
		SourceMod::sm_datatable_info_t info;

		datamap_t* datamap = gamehelpers->GetDataMap(entity);

		if (datamap == nullptr)
		{
			return TEAM_UNASSIGNED;
		}

		if (!gamehelpers->FindDataMapInfo(datamap, "m_iTeamNum", &info))
		{
			return TEAM_UNASSIGNED;
		}

		s_offset = static_cast<int>(info.actual_offset);
	}

	return *(int*)((uint8_t*)entity + s_offset);
}

std::int8_t entityprops::GetEntityLifeState(CBaseEntity* entity)
{
	/*
	 * This function should be pretty safe to use, on TF2 and CSS datamap dump, m_lifestate offset was the same for every entity.
	 * TO-DO: This is generally a signed char, if some mods change the variable type, type detection will be required for this.
	*/

	static int s_offset = -1;

	if (s_offset < 0)
	{
		SourceMod::sm_datatable_info_t info;
		if (!gamehelpers->FindDataMapInfo(gamehelpers->GetDataMap(entity), "m_lifeState", &info))
		{
			return LIFE_ALIVE;
		}

		if (info.prop->fieldType != FIELD_CHARACTER)
		{
			// this will warn if any mods have changed the variable type
			smutils->LogError(myself, "[NAVBOT] m_lifeState datamap is not a FIELD_CHARACTER!");
		}

		s_offset = static_cast<int>(info.actual_offset);
	}

	return *(std::int8_t*)((uint8_t*)entity + s_offset);
}

int entityprops::GetEntityHealth(CBaseEntity* entity)
{
	/* Like life state, the offset was the same for every entity in the game (TF2, CSS) */

	static int s_offset = -1;

	if (s_offset < 0)
	{
		SourceMod::sm_datatable_info_t info;
		if (!gamehelpers->FindDataMapInfo(gamehelpers->GetDataMap(entity), "m_iHealth", &info))
		{
			return LIFE_ALIVE;
		}

		s_offset = static_cast<int>(info.actual_offset);
	}

	return *(int*)((uint8_t*)entity + s_offset);
}

std::int8_t entityprops::GetEntityWaterLevel(CBaseEntity* entity)
{
	/*
	 * Offset is the same for every entity on TF2 datamap dump.
	 * TO-DO: This is generally a signed char, if some mods change the variable type, type detection will be required for this.
	*/

	static int s_offset = -1;

	if (s_offset < 0)
	{
		SourceMod::sm_datatable_info_t info;
		if (!gamehelpers->FindDataMapInfo(gamehelpers->GetDataMap(entity), "m_nWaterLevel", &info))
		{
			return LIFE_ALIVE;
		}

		if (info.prop->fieldType != FIELD_CHARACTER)
		{
			// this will warn if any mods have changed the variable type
			smutils->LogError(myself, "[NAVBOT] m_nWaterLevel datamap is not a FIELD_CHARACTER!");
		}

		s_offset = static_cast<int>(info.actual_offset);
	}

	return *(std::int8_t*)((uint8_t*)entity + s_offset);
}

const char* entityprops::GetEntityTargetname(CBaseEntity* entity)
{
	static int s_offset = -1;

	if (s_offset < 0)
	{
		CBaseEntity* worldspawn = gamehelpers->ReferenceToEntity(0);

		if (!worldspawn)
		{
			smutils->LogError(myself, "entityprops::GetEntityTargetname -- Failed to get a CBaseEntity ptr to worldspawn!");
			worldspawn = entity; // use the given entity if it fails
		}

		// Use worldspawn for the look up
		SourceMod::sm_datatable_info_t info;
		if (!gamehelpers->FindDataMapInfo(gamehelpers->GetDataMap(worldspawn), "m_iName", &info))
		{
			return nullptr;
		}

		s_offset = static_cast<int>(info.actual_offset);
	}

	string_t s = *(string_t*)((uint8_t*)entity + s_offset);

	if (s == NULL_STRING)
	{
		return nullptr;
	}

	return STRING(s);
}

void entityprops::GetEntityAbsVelocity(CBaseEntity* entity, Vector& result)
{
	static int s_offset = -1;

	if (s_offset < 0)
	{
		CBaseEntity* worldspawn = gamehelpers->ReferenceToEntity(0);

		if (!worldspawn)
		{
			smutils->LogError(myself, "entityprops::GetEntityAbsVelocity -- Failed to get a CBaseEntity ptr to worldspawn!");
			worldspawn = entity; // use the given entity if it fails
		}

		// Use worldspawn for the look up
		SourceMod::sm_datatable_info_t info;
		if (!gamehelpers->FindDataMapInfo(gamehelpers->GetDataMap(worldspawn), "m_vecAbsVelocity", &info))
		{
			return;
		}

		s_offset = static_cast<int>(info.actual_offset);
	}

	Vector* vel = (Vector*)((uint8_t*)entity + s_offset);

	result = *vel;

	return;
}

CStudioHdr* entityprops::GetEntityModelPtr(CBaseEntity* entity, const bool validate)
{
	static unsigned int offset = 0U;

	if (validate)
	{
		if (!entprops->HasEntProp(entity, Prop_Data, "m_OnIgnite"))
		{
			return nullptr;
		}
	}

	// initialize
	if (offset == 0U)
	{
		if (GamedataConstants::s_sizeof_COutputEvent == 0U)
		{
			return nullptr;
		}

		datamap_t* dmap = gamehelpers->GetDataMap(entity);

		if (!dmap)
		{
			return nullptr;
		}

		SourceMod::sm_datatable_info_t info;

		if (gamehelpers->FindDataMapInfo(dmap, "m_OnIgnite", &info))
		{
			offset = info.actual_offset + static_cast<unsigned int>(GamedataConstants::s_sizeof_COutputEvent);
		}
		else
		{
			return nullptr;
		}
	}

	// return reinterpret_cast<CStudioHdr*>(reinterpret_cast<std::intptr_t>(entity) + static_cast<std::intptr_t>(offset));
	return entprops->GetPointerToEntData<CStudioHdr>(entity, offset);
}
