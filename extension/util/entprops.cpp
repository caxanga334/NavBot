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

#include <util/helpers.h>
#include "entprops.h"

// From game/server/variant_t.h, same on all supported games.
class variant_t
{
public:
	union
	{
		bool bVal;
		string_t iszVal;
		int iVal;
		float flVal;
		float vecVal[3];
		color32 rgbaVal;
	};
	
	CBaseHandle eVal;
	fieldtype_t fieldType;
};

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
	if (!gamehelpers->FindSendPropInfo(grclassname, prop, &info)) \
	{ \
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

void CEntPropUtils::Init(bool reset)
{
	if (initialized && !reset)
		return;

	SourceMod::IGameConfig *gamedata;
	char *error = nullptr;
	size_t maxlength = 0;
	grclassname = nullptr;

	if (!gameconfs->LoadGameConfigFile("sdktools.games", &gamedata, error, maxlength))
	{
		smutils->LogError(myself, "CEntPropUtils::Init -- Failed to load sdktools.game from SourceMod gamedata.");
		return;
	}

	grclassname = gamedata->GetKeyValue("GameRulesProxy");

	if (!grclassname)
	{
		smutils->LogError(myself, "CEntPropUtils::Init -- Failed to get game rules proxy classname.");
	}
	else
	{
		smutils->LogMessage(myself, "CEntPropUtils::Init --  Retrieved game rules proxy classname \"%s\".", grclassname);
	}

	gameconfs->CloseGameConfigFile(gamedata);
	initialized = true;
	smutils->LogMessage(myself, "CEntPropUtils::Init -- Done.");
}

/// @brief Checks if the given entity is a networked entity
/// @param pEntity Entity to check
/// @return true if the entity is networked, false otherwise
bool CEntPropUtils::IsNetworkedEntity(CBaseEntity *pEntity)
{
	IServerUnknown *pUnk = (IServerUnknown *)pEntity;
	IServerNetworkable *pNet = pUnk->GetNetworkable();

	if (!pNet)
	{
		return false;
	}

	return true;
}

bool CEntPropUtils::FindSendProp(SourceMod::sm_sendprop_info_t *info, CBaseEntity *pEntity, const char *prop, int entity)
{
	IServerUnknown *pUnk = (IServerUnknown *)pEntity;
	IServerNetworkable *pNet = pUnk->GetNetworkable();

	if (!pNet)
	{
		return false;
	}

	if (!gamehelpers->FindSendPropInfo(pNet->GetServerClass()->GetName(), prop, info))
	{
		return false;
	}

	return true;
}

/* Given an entity reference or index, fill out a CBaseEntity and/or edict.
   If lookup is successful, returns true and writes back the two parameters.
   If lookup fails, returns false and doesn't touch the params.  */
bool CEntPropUtils::IndexToAThings(int num, CBaseEntity **pEntData, edict_t **pEdictData)
{
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

/// @brief Retrieves an integer value in an entity's property.
/// @param entity Entity/edict index.
/// @param proptype Property type.
/// @param prop Property name.
/// @param size Number of bytes to write (valid values are 1, 2, or 4). This value is auto-detected, and the size parameter is only used as a fallback in case detection fails.
/// @param element Element # (starting from 0) if property is an array.
/// @return Value at the given property offset.
int CEntPropUtils::GetEntProp(int entity, PropType proptype, const char *prop, int size, int element)
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
		return 0;
	}

	switch (proptype)
	{
	case Prop_Data:
		typedescription_t *td;
		datamap_t *pMap;

		if ((pMap = gamehelpers->GetDataMap(pEntity)) == NULL)
		{
			return 0;
		}

		SourceMod::sm_datatable_info_t dinfo;

		if (!gamehelpers->FindDataMapInfo(pMap, prop, &dinfo))
		{
			return 0;
		}

		td = dinfo.prop;

		if ((bit_count = MatchTypeDescAsInteger(td->fieldType, td->flags)) == 0)
		{
			return 0;
		}

		CHECK_SET_PROP_DATA_OFFSET(0);
		
		if (td->fieldType == FIELD_CUSTOM && (td->flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT)
		{
			auto *pVariant = (variant_t *)((intptr_t)pEntity + offset);
			if ((bit_count = MatchTypeDescAsInteger(pVariant->fieldType, 0)) == 0)
			{
				return 0;
			}
		}

		break;

	case Prop_Send:
		
		if (!FindSendProp(&info, pEntity, prop, entity))
		{
			return 0;
		}

		offset = info.actual_offset;
		pProp = info.prop;
		bit_count = pProp->m_nBits;

		PROP_TYPE_SWITCH(DPT_Int, "integer", 0);

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
		return 0;
		break;
	}

	if (bit_count < 1)
	{
		bit_count = size * 8;
	}

	if (bit_count >= 17)
	{
		return *(int32_t *)((uint8_t *)pEntity + offset);
	}
	else if (bit_count >= 9)
	{
		if (is_unsigned)
		{
			return *(uint16_t *)((uint8_t *)pEntity + offset);
		}
		else
		{
			return *(int16_t *)((uint8_t *)pEntity + offset);
		}
	}
	else if (bit_count >= 2)
	{
		if (is_unsigned)
		{
			return *(uint8_t *)((uint8_t *)pEntity + offset);
		}
		else
		{
			return *(int8_t *)((uint8_t *)pEntity + offset);
		}
	}
	else
	{
		return (bool *)((uint8_t *)pEntity + offset) ? 1 : 0;
	}
}

/// @brief Retrieves an integer pointer in an entity's property.
/// @param entity Entity/edict index.
/// @param proptype Property type.
/// @param prop Property name.
/// @param size Number of bytes to write (valid values are 1, 2, or 4). This value is auto-detected, and the size parameter is only used as a fallback in case detection fails.
/// @param element Element # (starting from 0) if property is an array.
/// @return Pointer at the given property offset.
int *CEntPropUtils::GetEntPropPointer(int entity, PropType proptype, const char *prop, int size, int element)
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
		return nullptr;
	}

	switch (proptype)
	{
	case Prop_Data:
		typedescription_t *td;
		datamap_t *pMap;

		if ((pMap = gamehelpers->GetDataMap(pEntity)) == NULL)
		{
			return nullptr;
		}

		SourceMod::sm_datatable_info_t dinfo;

		if (!gamehelpers->FindDataMapInfo(pMap, prop, &dinfo))
		{
			return nullptr;
		}

		td = dinfo.prop;

		if ((bit_count = MatchTypeDescAsInteger(td->fieldType, td->flags)) == 0)
		{
			return nullptr;
		}

		CHECK_SET_PROP_DATA_OFFSET(0);
		
		if (td->fieldType == FIELD_CUSTOM && (td->flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT)
		{
			auto *pVariant = (variant_t *)((intptr_t)pEntity + offset);
			if ((bit_count = MatchTypeDescAsInteger(pVariant->fieldType, 0)) == 0)
			{
				return nullptr;
			}
		}

		break;

	case Prop_Send:
		
		if (!FindSendProp(&info, pEntity, prop, entity))
		{
			return nullptr;
		}

		offset = info.actual_offset;
		pProp = info.prop;
		bit_count = pProp->m_nBits;

		PROP_TYPE_SWITCH(DPT_Int, "integer", nullptr);

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
		break;
	}

	if (bit_count < 1)
	{
		bit_count = size * 8;
	}

	if (bit_count >= 17)
	{
		return (int32_t *)((uint8_t *)pEntity + offset);
	}
	else if (bit_count >= 9)
	{
		if (is_unsigned)
		{
			return (int*)(uint16_t *)((uint8_t *)pEntity + offset);
		}
		else
		{
			return (int*)(int16_t *)((uint8_t *)pEntity + offset);
		}
	}
	else if (bit_count >= 2)
	{
		if (is_unsigned)
		{
			return (int*)(uint8_t *)((uint8_t *)pEntity + offset);
		}
		else
		{
			return (int*)(int8_t *)((uint8_t *)pEntity + offset);
		}
	}
	else
	{
		return (int*)((uint8_t *)pEntity + offset);
	}
}

/// @brief Retrieves a boolean value in an entity's property.
/// @param entity Entity/edict index.
/// @param proptype Property type.
/// @param prop Property name.
/// @param element Element # (starting from 0) if property is an array.
/// @return Value at the given property offset.
bool CEntPropUtils::GetEntPropBool(int entity, PropType proptype, const char *prop, int element)
{
	return GetEntProp(entity, proptype, prop, 1, element) != 0;
}

/// @brief Retrieves a boolean pointer in an entity's property.
/// @attention This will return nullptr if the property bit_count is higher than 1. Use SourceMod's sm_dump_netprops to double check the property
/// you are accessing has a size of 1. Or just use GetEntPropPointer instead.
/// @param entity Entity/edict index.
/// @param proptype Property type.
/// @param prop Property name.
/// @param element Element # (starting from 0) if property is an array.
/// @return Pointer at the given property offset.
bool *CEntPropUtils::GetEntPropBoolPointer(int entity, PropType proptype, const char *prop, int element)
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
		return nullptr;
	}

	switch (proptype)
	{
	case Prop_Data:
		typedescription_t *td;
		datamap_t *pMap;

		if ((pMap = gamehelpers->GetDataMap(pEntity)) == NULL)
		{
			return nullptr;
		}

		SourceMod::sm_datatable_info_t dinfo;

		if (!gamehelpers->FindDataMapInfo(pMap, prop, &dinfo))
		{
			return nullptr;
		}

		td = dinfo.prop;

		if ((bit_count = MatchTypeDescAsInteger(td->fieldType, td->flags)) == 0)
		{
			return nullptr;
		}

		CHECK_SET_PROP_DATA_OFFSET(0);
		
		if (td->fieldType == FIELD_CUSTOM && (td->flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT)
		{
			auto *pVariant = (variant_t *)((intptr_t)pEntity + offset);
			if ((bit_count = MatchTypeDescAsInteger(pVariant->fieldType, 0)) == 0)
			{
				return nullptr;
			}
		}

		break;

	case Prop_Send:
		
		if (!FindSendProp(&info, pEntity, prop, entity))
		{
			return nullptr;
		}

		offset = info.actual_offset;
		pProp = info.prop;
		bit_count = pProp->m_nBits;

		PROP_TYPE_SWITCH(DPT_Int, "integer", nullptr);

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
		return nullptr;
		break;
	}

	if (bit_count < 1)
	{
		bit_count = 1 * 8;
	}

	if (bit_count > 1)
	{
		return nullptr;
	}
	else
	{
		return (bool*)((uint8_t *)pEntity + offset);
	}
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
		typedescription_t *td;
		datamap_t *pMap;

		if ((pMap = gamehelpers->GetDataMap(pEntity)) == NULL)
		{
			return false;
		}

		SourceMod::sm_datatable_info_t dinfo;

		if (!gamehelpers->FindDataMapInfo(pMap, prop, &dinfo))
		{
			const char *classname = gamehelpers->GetEntityClassname(pEntity);
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
		
		if (!FindSendProp(&info, pEntity, prop, entity))
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

/// @brief Sets a float value in an entity's property.
/// @param entity Entity/edict index.
/// @param proptype Property type.
/// @param prop Property name.
/// @param element Element # (starting from 0) if property is an array.
/// @return Value at the given property offset.
float CEntPropUtils::GetEntPropFloat(int entity, PropType proptype, const char *prop, int element)
{
	edict_t *pEdict;
	CBaseEntity *pEntity;
	SourceMod::sm_sendprop_info_t info;
	SendProp *pProp = nullptr;
	int bit_count;
	int offset;

	if (!IndexToAThings(entity, &pEntity, &pEdict))
	{
		return 0.0f;
	}

	switch (proptype)
	{
	case Prop_Data:
		typedescription_t *td;
		datamap_t *pMap;

		if ((pMap = gamehelpers->GetDataMap(pEntity)) == NULL)
		{
			return 0.0f;
		}

		SourceMod::sm_datatable_info_t dinfo;

		if (!gamehelpers->FindDataMapInfo(pMap, prop, &dinfo))
		{
			const char *classname = gamehelpers->GetEntityClassname(pEntity);
			return 0.0f;
		}

		td = dinfo.prop;

		CHECK_SET_PROP_DATA_OFFSET(0.0f);
		
		CHECK_TYPE_VALID_IF_VARIANT(FIELD_FLOAT, "float", 0.0f);

		break;

	case Prop_Send:
		
		if (!FindSendProp(&info, pEntity, prop, entity))
		{
			return 0.0f;
		}

		offset = info.actual_offset;
		pProp = info.prop;
		bit_count = pProp->m_nBits;

		PROP_TYPE_SWITCH(DPT_Float, "float", 0.0f);

		break;
	
	default:
		return 0.0f;
		break;
	}

	return *(float *)((uint8_t *)pEntity + offset);
}

/// @brief Retrieves a float pointer in an entity's property.
/// @param entity Entity/edict index.
/// @param proptype Property type.
/// @param prop Property name.
/// @param element Element # (starting from 0) if property is an array.
/// @return Pointer at the given property offset.
float *CEntPropUtils::GetEntPropFloatPointer(int entity, PropType proptype, const char *prop, int element)
{
	edict_t *pEdict;
	CBaseEntity *pEntity;
	SourceMod::sm_sendprop_info_t info;
	SendProp *pProp = nullptr;
	int bit_count;
	int offset;

	if (!IndexToAThings(entity, &pEntity, &pEdict))
	{
		return nullptr;
	}

	switch (proptype)
	{
	case Prop_Data:
		typedescription_t *td;
		datamap_t *pMap;

		if ((pMap = gamehelpers->GetDataMap(pEntity)) == NULL)
		{
			return nullptr;
		}

		SourceMod::sm_datatable_info_t dinfo;

		if (!gamehelpers->FindDataMapInfo(pMap, prop, &dinfo))
		{
			return nullptr;
		}

		td = dinfo.prop;

		CHECK_SET_PROP_DATA_OFFSET(nullptr);
		
		CHECK_TYPE_VALID_IF_VARIANT(FIELD_FLOAT, "float", nullptr);

		break;

	case Prop_Send:
		
		if (!FindSendProp(&info, pEntity, prop, entity))
		{
			return nullptr;
		}

		offset = info.actual_offset;
		pProp = info.prop;
		bit_count = pProp->m_nBits;

		PROP_TYPE_SWITCH(DPT_Float, "float", nullptr);

		break;
	
	default:
		return nullptr;
		break;
	}

	return (float *)((uint8_t *)pEntity + offset);
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
		typedescription_t *td;
		datamap_t *pMap;

		if ((pMap = gamehelpers->GetDataMap(pEntity)) == NULL)
		{
			return false;
		}

		SourceMod::sm_datatable_info_t dinfo;

		if (!gamehelpers->FindDataMapInfo(pMap, prop, &dinfo))
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
		
		if (!FindSendProp(&info, pEntity, prop, entity))
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

/// @brief Retrieves an entity index from an entity's property.
/// @param entity Entity/edict index.
/// @param proptype Property type.
/// @param prop Property name.
/// @param element Element # (starting from 0) if property is an array.
/// @return Entity index at the given property. If there is no entity, or the entity is not valid, then -1 is returned.
int CEntPropUtils::GetEntPropEnt(int entity, PropType proptype, const char *prop, int element)
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
		return -1;
	}

	switch (proptype)
	{
	case Prop_Data:
		typedescription_t *td;
		datamap_t *pMap;

		if ((pMap = gamehelpers->GetDataMap(pEntity)) == NULL)
		{
			return -1;
		}

		SourceMod::sm_datatable_info_t dinfo;

		if (!gamehelpers->FindDataMapInfo(pMap, prop, &dinfo))
		{
			return -1;
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
			return -1;
		}

		CHECK_SET_PROP_DATA_OFFSET(0);

		CHECK_TYPE_VALID_IF_VARIANT(FIELD_EHANDLE, "ehandle", 0);

		break;

	case Prop_Send:
		
		type = PropEnt_Handle;

		if (!FindSendProp(&info, pEntity, prop, entity))
		{
			return -1;
		}

		offset = info.actual_offset;
		pProp = info.prop;
		bit_count = pProp->m_nBits;

		PROP_TYPE_SWITCH(DPT_Int, "integer", -1);

		break;
	
	default:
		return -1;
		break;
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

			CBaseEntity *pHandleEntity = gamehelpers->ReferenceToEntity(hndl->GetEntryIndex());

			if (!pHandleEntity || *hndl != reinterpret_cast<IHandleEntity *>(pHandleEntity)->GetRefEHandle())
				return -1;

			return gamehelpers->EntityToBCompatRef(pHandleEntity);
		}
	case PropEnt_Entity:
		{
			CBaseEntity *pPropEntity = *(CBaseEntity **) ((uint8_t *) pEntity + offset);
			return gamehelpers->EntityToBCompatRef(pPropEntity);
		}
	case PropEnt_Edict:
		{
			edict_t *_pEdict = *(edict_t **) ((uint8_t *) pEntity + offset);
			if (!_pEdict || _pEdict->IsFree())
				return -1;

			return  gamehelpers->IndexOfEdict(_pEdict);
		}
	}

	return -1;
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
		typedescription_t *td;
		datamap_t *pMap;

		if ((pMap = gamehelpers->GetDataMap(pEntity)) == NULL)
		{
			return false;
		}

		SourceMod::sm_datatable_info_t dinfo;

		if (!gamehelpers->FindDataMapInfo(pMap, prop, &dinfo))
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

		if (!FindSendProp(&info, pEntity, prop, entity))
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
Vector CEntPropUtils::GetEntPropVector(int entity, PropType proptype, const char *prop, int element)
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
		return Vector(0,0,0);
	}

	switch (proptype)
	{
	case Prop_Data:
		typedescription_t *td;
		datamap_t *pMap;

		if ((pMap = gamehelpers->GetDataMap(pEntity)) == NULL)
		{
			return Vector(0,0,0);
		}

		SourceMod::sm_datatable_info_t dinfo;

		if (!gamehelpers->FindDataMapInfo(pMap, prop, &dinfo))
		{
			return Vector(0,0,0);
		}

		td = dinfo.prop;

		if (td->fieldType != FIELD_VECTOR && td->fieldType != FIELD_POSITION_VECTOR)
		{
			return Vector(0,0,0);
		}

		CHECK_SET_PROP_DATA_OFFSET(Vector(0,0,0));
		
		if (td->fieldType == FIELD_CUSTOM && (td->flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT)
		{
			auto *pVariant = (variant_t *)((intptr_t)pEntity + offset);
			if (pVariant->fieldType != FIELD_VECTOR && pVariant->fieldType != FIELD_POSITION_VECTOR)
			{
				return Vector(0,0,0);
			}
		}

		break;

	case Prop_Send:
		
		if (!FindSendProp(&info, pEntity, prop, entity))
		{
			return Vector(0,0,0);
		}

		offset = info.actual_offset;
		pProp = info.prop;
		bit_count = pProp->m_nBits;

		PROP_TYPE_SWITCH(DPT_Vector, "vector", Vector(0,0,0));

		break;
	
	default:
		return Vector(0,0,0);
		break;
	}

	Vector *v = (Vector *)((uint8_t *)pEntity + offset);

	return *v;
}

/// @brief Retrieves a vector from an entity, given a named network property.
/// @param entity Entity/edict index.
/// @param proptype Property type.
/// @param prop Property name.
/// @param element Element # (starting from 0) if property is an array.
/// @return Vector pointer from the property
Vector *CEntPropUtils::GetEntPropVectorPointer(int entity, PropType proptype, const char *prop, int element)
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
		return nullptr;
	}

	switch (proptype)
	{
	case Prop_Data:
		typedescription_t *td;
		datamap_t *pMap;

		if ((pMap = gamehelpers->GetDataMap(pEntity)) == NULL)
		{
			return nullptr;
		}

		SourceMod::sm_datatable_info_t dinfo;

		if (!gamehelpers->FindDataMapInfo(pMap, prop, &dinfo))
		{
			return nullptr;
		}

		td = dinfo.prop;

		if (td->fieldType != FIELD_VECTOR && td->fieldType != FIELD_POSITION_VECTOR)
		{
			return nullptr;
		}

		CHECK_SET_PROP_DATA_OFFSET(nullptr);
		
		if (td->fieldType == FIELD_CUSTOM && (td->flags & FTYPEDESC_OUTPUT) == FTYPEDESC_OUTPUT)
		{
			auto *pVariant = (variant_t *)((intptr_t)pEntity + offset);
			if (pVariant->fieldType != FIELD_VECTOR && pVariant->fieldType != FIELD_POSITION_VECTOR)
			{
				return nullptr;
			}
		}

		break;

	case Prop_Send:
		
		if (!FindSendProp(&info, pEntity, prop, entity))
		{
			return nullptr;
		}

		offset = info.actual_offset;
		pProp = info.prop;
		bit_count = pProp->m_nBits;

		PROP_TYPE_SWITCH(DPT_Vector, "vector", nullptr);

		break;
	
	default:
		return nullptr;
		break;
	}

	Vector *v = (Vector *)((uint8_t *)pEntity + offset);

	return v;
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
		typedescription_t *td;
		datamap_t *pMap;

		if ((pMap = gamehelpers->GetDataMap(pEntity)) == NULL)
		{
			return false;
		}

		SourceMod::sm_datatable_info_t dinfo;

		if (!gamehelpers->FindDataMapInfo(pMap, prop, &dinfo))
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
		
		if (!FindSendProp(&info, pEntity, prop, entity))
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
char *CEntPropUtils::GetEntPropString(int entity, PropType proptype, const char *prop, int maxlen, int *len, int element)
{
	edict_t *pEdict;
	CBaseEntity *pEntity;
	SourceMod::sm_sendprop_info_t info;
	SendProp *pProp = nullptr;
	int bit_count;
	int offset;
	const char *src = nullptr;
	char *dest = nullptr;
	bool bIsStringIndex = false;

	if (!IndexToAThings(entity, &pEntity, &pEdict))
	{
		return nullptr;
	}

	switch (proptype)
	{
	case Prop_Data:
		typedescription_t *td;
		datamap_t *pMap;

		if ((pMap = gamehelpers->GetDataMap(pEntity)) == NULL)
		{
			return nullptr;
		}

		SourceMod::sm_datatable_info_t dinfo;

		if (!gamehelpers->FindDataMapInfo(pMap, prop, &dinfo))
		{
			return nullptr;
		}

		td = dinfo.prop;

		if ((td->fieldType != FIELD_CHARACTER
			&& td->fieldType != FIELD_STRING
			&& td->fieldType != FIELD_MODELNAME 
			&& td->fieldType != FIELD_SOUNDNAME)
			|| (td->fieldType == FIELD_CUSTOM && (td->flags & FTYPEDESC_OUTPUT) != FTYPEDESC_OUTPUT))
		{
			return nullptr;
		}

		bIsStringIndex = (td->fieldType != FIELD_CHARACTER);

		if (element != 0)
		{
			if (bIsStringIndex)
			{
				if (element < 0 || element >= td->fieldSize)
				{
					return nullptr;
				}
			}
			else
			{
				return nullptr;
			}
		}

		offset = dinfo.actual_offset;

		if (bIsStringIndex)
		{
			offset += (element * (td->fieldSizeInBytes / td->fieldSize));

			string_t idx;

			idx = *(string_t *) ((uint8_t *) pEntity + offset);
			src = (idx == NULL_STRING) ? "" : STRING(idx);
		}
		else
		{
			src = (char *) ((uint8_t *) pEntity + offset);
		}

		break;

	case Prop_Send:
		
		if (!FindSendProp(&info, pEntity, prop, entity))
		{
			return nullptr;
		}

		offset = info.actual_offset;
		pProp = info.prop;
		bit_count = pProp->m_nBits;

		PROP_TYPE_SWITCH(DPT_String, "string", nullptr);

		if (pProp->GetProxyFn())
		{
			DVariant var;
			pProp->GetProxyFn()(pProp, pEntity, (const void *) ((intptr_t) pEntity + offset), &var, element, entity);
			src = (char*)var.m_pString; // hack because SDK 2013 declares this as const char*
		}
		else
		{
			src = *(char **) ((uint8_t *) pEntity + offset);
		}

		break;
	
	default:
		return nullptr;
		break;
	}

	size_t length = ke::SafeStrcpy(dest, maxlen, src);
	*len = length;

	return dest;
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
int CEntPropUtils::GetEntData(int entity, int offset, int size)
{
	CBaseEntity *pEntity = GetEntity(entity);

	if (!pEntity)
	{
		return 0;
	}

	if (offset <= 0 || offset > 32768)
	{
		return 0;
	}

	switch (size)
	{
	case 4:
		return *(int *)((uint8_t *)pEntity + offset);
	case 2:
		return *(short *)((uint8_t *)pEntity + offset);
	case 1:
		return *((uint8_t *)pEntity + offset);
	default:
		return 0;
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
float CEntPropUtils::GetEntDataFloat(int entity, int offset)
{
	CBaseEntity *pEntity = GetEntity(entity);

	if (!pEntity)
	{
		return 0.0f;
	}

	if (offset <= 0 || offset > 32768)
	{
		return 0.0f;
	}

	return *(float *)((uint8_t *)pEntity + offset);
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
int CEntPropUtils::GetEntDataEnt(int entity, int offset)
{
	CBaseEntity *pEntity = GetEntity(entity);

	if (!pEntity)
	{
		return -1;
	}

	if (offset <= 0 || offset > 32768)
	{
		return -1;
	}

	CBaseHandle &hndl = *(CBaseHandle *)((uint8_t *)pEntity + offset);
	CBaseEntity *pHandleEntity = gamehelpers->ReferenceToEntity(hndl.GetEntryIndex());

	if (!pHandleEntity || hndl != reinterpret_cast<IHandleEntity *>(pHandleEntity)->GetRefEHandle())
		return -1;

	return gamehelpers->EntityToBCompatRef(pHandleEntity);
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
Vector CEntPropUtils::GetEntDataVector(int entity, int offset)
{
	CBaseEntity *pEntity = GetEntity(entity);

	if (!pEntity)
	{
		return Vector(0,0,0);
	}

	if (offset <= 0 || offset > 32768)
	{
		return Vector(0,0,0);
	}

	Vector *v = (Vector *)((uint8_t *)pEntity + offset);

	return *v;
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
char *CEntPropUtils::GetEntDataString(int entity, int offset, int maxlen, int *len)
{
	CBaseEntity *pEntity = GetEntity(entity);

	if (!pEntity)
	{
		return nullptr;
	}

	if (offset <= 0 || offset > 32768)
	{
		return nullptr;
	}

	if (maxlen <= 0)
	{
		return nullptr;
	}

	char *src = (char *)((uint8_t *)pEntity + offset);
	char *dest = nullptr;
	size_t length = ke::SafeStrcpy(dest, maxlen, src);
	*len = length;
	return dest;
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

CBaseEntity *CEntPropUtils::GetGameRulesProxyEntity()
{
	static int proxyEntRef = -1;
	CBaseEntity *pProxy;
	if (proxyEntRef == -1 || (pProxy = gamehelpers->ReferenceToEntity(proxyEntRef)) == NULL)
	{
		pProxy = GetEntity(UtilHelpers::FindEntityByNetClass(playerhelpers->GetMaxClients(), grclassname));
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
int CEntPropUtils::GameRules_GetProp(const char *prop, int size, int element)
{
	int offset;
	int bit_count;
	bool is_unsigned = false;
	void *pGameRules = g_pSDKTools->GetGameRules();

	if (!pGameRules || !grclassname || !strcmp(grclassname, ""))
	{
		return -1;
	}

	int elementCount = 1;
	GAMERULES_FIND_PROP_SEND(DPT_Int, "integer", -1);
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
		return *(int32_t *)((intptr_t)pGameRules + offset);
	}
	else if (bit_count >= 9)
	{
		if (is_unsigned)
		{
			return *(uint16_t *)((intptr_t)pGameRules + offset);
		}
		else
		{
			return *(int16_t *)((intptr_t)pGameRules + offset);
		}
	}
	else if (bit_count >= 2)
	{
		if (is_unsigned)
		{
			return *(uint8_t *)((intptr_t)pGameRules + offset);
		}
		else
		{
			return *(int8_t *)((intptr_t)pGameRules + offset);
		}
	}
	else
	{
		return *(bool *)((intptr_t)pGameRules + offset) ? 1 : 0;
	}

	return -1;
}

/// @brief Retrieves a float value from a property of the gamerules entity.
/// @param prop Property name.
/// @param element Element # (starting from 0) if property is an array.
/// @return Value at the given property offset.
float CEntPropUtils::GameRules_GetPropFloat(const char *prop, int element)
{
	int offset;
	int bit_count;
	void *pGameRules = g_pSDKTools->GetGameRules();

	if (!pGameRules || !grclassname || !strcmp(grclassname, ""))
	{
		return 0.0f;
	}

	int elementCount = 1;
	GAMERULES_FIND_PROP_SEND(DPT_Float, "float", 0.0f);

	return *(float *)((intptr_t)pGameRules + offset);
}

/// @brief Retrieves a entity index from a property of the gamerules entity.
/// @param prop Property name.
/// @param element Element # (starting from 0) if property is an array.
/// @return Entity index at the given property. If there is no entity, or the entity is not valid, then -1 is returned.
int CEntPropUtils::GameRules_GetPropEnt(const char *prop, int element)
{
	int offset;
	int bit_count;
	void *pGameRules = g_pSDKTools->GetGameRules();

	if (!pGameRules || !grclassname || !strcmp(grclassname, ""))
	{
		return 0.0f;
	}

	int elementCount = 1;
	GAMERULES_FIND_PROP_SEND(DPT_Int, "Integer", 0.0f);

	CBaseHandle &hndl = *(CBaseHandle *)((intptr_t)pGameRules + offset);
	CBaseEntity *pEntity = gamehelpers->ReferenceToEntity(hndl.GetEntryIndex());

	if (!pEntity || ((IServerEntity *)pEntity)->GetRefEHandle() != hndl)
	{
		return -1;
	}

	return gamehelpers->EntityToBCompatRef(pEntity);
}

/// @brief Retrieves a vector from the gamerules entity, given a named network property.
/// @param prop Property name.
/// @param element Element # (starting from 0) if property is an array.
/// @return Value at the given property offset.
Vector CEntPropUtils::GameRules_GetPropVector(const char *prop, int element)
{
	int offset;
	int bit_count;
	void *pGameRules = g_pSDKTools->GetGameRules();

	if (!pGameRules || !grclassname || !strcmp(grclassname, ""))
	{
		return Vector(0,0,0);
	}

	int elementCount = 1;
	GAMERULES_FIND_PROP_SEND(DPT_Vector, "vector", Vector(0,0,0));

	return *(Vector *)((intptr_t)pGameRules + offset);
}

/// @brief Gets a gamerules property as a string.
/// @param prop Property to use.
/// @param len Number of non-null bytes written.
/// @param maxlen Maximum length of output string buffer.
/// @param element Element # (starting from 0) if property is an array.
/// @return Value at the given property offset.
char *CEntPropUtils::GameRules_GetPropString(const char *prop, int *len, int maxlen, int element)
{
	int offset;
	int bit_count;
	void *pGameRules = g_pSDKTools->GetGameRules();

	if (!pGameRules || !grclassname || !strcmp(grclassname, ""))
	{
		return nullptr;
	}

	int elementCount = 1;
	GAMERULES_FIND_PROP_SEND(DPT_String, "string", nullptr);


	const char *src;
	char *dest = nullptr;
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
		size_t length = ke::SafeStrcpy(dest, maxlen, src);
		*len = length;
	}

	return dest;
}

RoundState CEntPropUtils::GameRules_GetRoundState()
{
	return static_cast<RoundState>(GameRules_GetProp("m_iRoundState"));
}