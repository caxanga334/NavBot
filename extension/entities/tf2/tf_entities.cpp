#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <mods/tf2/tf2lib.h>
#include "tf_entities.h"

tfentities::HTFBaseEntity::HTFBaseEntity(edict_t* entity) : entities::HBaseEntity(entity)
{
}

tfentities::HTFBaseEntity::HTFBaseEntity(CBaseEntity* entity) : entities::HBaseEntity(entity)
{
}

TeamFortress2::TFTeam tfentities::HTFBaseEntity::GetTFTeam() const
{
	return tf2lib::GetEntityTFTeam(GetIndex());
}

tfentities::HCaptureFlag::HCaptureFlag(edict_t* entity) : HTFBaseEntity(entity)
{
}

tfentities::HCaptureFlag::HCaptureFlag(CBaseEntity* entity) : HTFBaseEntity(entity)
{
}

bool tfentities::HCaptureFlag::IsDisabled() const
{
	bool result = false;
	entprops->GetEntPropBool(GetIndex(), Prop_Send, "m_bDisabled", result);
	return result;
}

TeamFortress2::TFFlagType tfentities::HCaptureFlag::GetType() const
{
	int type = 0;
	entprops->GetEntProp(GetIndex(), Prop_Send, "m_nType", type);
	return static_cast<TeamFortress2::TFFlagType>(type);
}

int tfentities::HCaptureFlag::GetStatus() const
{
	int status = 0;
	entprops->GetEntProp(GetIndex(), Prop_Send, "m_nFlagStatus", status);
	return status;
}

// Returns the actual position of the flag
Vector tfentities::HCaptureFlag::GetPosition() const
{
	auto owner = GetOwnerEntity();

	if (owner) // flag has an owner, probably is being carried by a player, return the player position
	{
		HBaseEntity eOwner(owner);
		return eOwner.GetAbsOrigin();
	}

	return GetAbsOrigin(); // no owner, return my position
}

Vector tfentities::HCaptureFlag::GetReturnPosition() const
{
	using namespace SourceMod;
	static int offset = -1;
	CBaseEntity* entity = gamehelpers->ReferenceToEntity(GetIndex());

	if (offset < 0)
	{
		IGameConfig* cfg = extension->GetExtensionGameData();
		int add_to_offset;

		if (!cfg->GetOffset("CCaptureFlag::m_vecResetPos", &add_to_offset))
		{
			smutils->LogError(myself, "Failed to get offset for CCaptureFlag::m_vecResetPos from NavBot's gamedata file!");
		}

		
		int base;

		if (UtilHelpers::FindSendPropOffset(entity, "m_flTimeToSetPoisonous", base))
		{
			offset = base + add_to_offset;
		}
	}

	if (offset < 0)
	{
		return vec3_origin;
	}

	Vector* result = (Vector*)((uint8_t*)entity + offset);
	return *result;
}

tfentities::HCaptureZone::HCaptureZone(edict_t* entity) : HTFBaseEntity(entity)
{
}

tfentities::HCaptureZone::HCaptureZone(CBaseEntity* entity) : HTFBaseEntity(entity)
{
}

bool tfentities::HCaptureZone::IsDisabled() const
{
	bool result = false;
	entprops->GetEntPropBool(GetIndex(), Prop_Send, "m_bDisabled", result);
	return result;
}

tfentities::HFuncRegenerate::HFuncRegenerate(edict_t* entity) : HTFBaseEntity(entity)
{
}

tfentities::HFuncRegenerate::HFuncRegenerate(CBaseEntity* entity) : HTFBaseEntity(entity)
{
}

bool tfentities::HFuncRegenerate::IsDisabled() const
{
	bool result = false;
	entprops->GetEntPropBool(GetIndex(), Prop_Data, "m_bDisabled", result);
	return result;
}

tfentities::HBaseObject::HBaseObject(edict_t* entity) : HTFBaseEntity(entity)
{
}

tfentities::HBaseObject::HBaseObject(CBaseEntity* entity) : HTFBaseEntity(entity)
{
}

int tfentities::HBaseObject::GetHealth() const
{
	int value = 0;
	entprops->GetEntProp(GetIndex(), Prop_Send, "m_iHealth", value);
	return value;
}

int tfentities::HBaseObject::GetMaxHealth() const
{
	int value = 0;
	entprops->GetEntProp(GetIndex(), Prop_Send, "m_iMaxHealth", value);
	return value;
}

bool tfentities::HBaseObject::IsDisabled() const
{
	bool result = false;
	entprops->GetEntPropBool(GetIndex(), Prop_Send, "m_bDisabled", result);
	return result;
}

float tfentities::HBaseObject::GetPercentageConstructed() const
{
	float value = 0.0f;
	entprops->GetEntPropFloat(GetIndex(), Prop_Send, "m_flPercentageConstructed", value);
	return value;
}

TeamFortress2::TFObjectType tfentities::HBaseObject::GetType() const
{
	int value = 0;
	entprops->GetEntProp(GetIndex(), Prop_Send, "m_iObjectType", value);
	return static_cast<TeamFortress2::TFObjectType>(value);
}

TeamFortress2::TFObjectMode tfentities::HBaseObject::GetMode() const
{
	int value = 0;
	entprops->GetEntProp(GetIndex(), Prop_Send, "m_iObjectMode", value);
	return static_cast<TeamFortress2::TFObjectMode>(value);
}

edict_t* tfentities::HBaseObject::GetBuilder() const
{
	int entity = INVALID_EHANDLE_INDEX;
	entprops->GetEntPropEnt(GetIndex(), Prop_Send, "m_hBuilder", entity);
	return gamehelpers->EdictOfIndex(entity);
}

int tfentities::HBaseObject::GetBuilderIndex() const
{
	int entity = INVALID_EHANDLE_INDEX;
	entprops->GetEntPropEnt(GetIndex(), Prop_Send, "m_hBuilder", entity);
	return entity;
}

int tfentities::HBaseObject::GetLevel() const
{
	int value = 0;
	entprops->GetEntProp(GetIndex(), Prop_Send, "m_iUpgradeLevel", value);
	return value;
}

int tfentities::HBaseObject::GetMaxLevel() const
{
	/* the actual max level isn't readable from a netprop, if we want to get it, we need to do a vcall
	int value = 0;
	entprops->GetEntProp(GetIndex(), Prop_Send, "m_iHighestUpgradeLevel", value);
	return value;
	*/

	/*
	* Unless we setup a vcall to GetMaxUpgradeLevel, we will have to assume these values
	*/

	if (IsMiniBuilding())
	{
		return 1;
	}
	else
	{
		return 3;
	}
}

bool tfentities::HBaseObject::IsPlacing() const
{
	bool result = false;
	entprops->GetEntPropBool(GetIndex(), Prop_Send, "m_bPlacing", result);
	return result;
}

bool tfentities::HBaseObject::IsBuilding() const
{
	bool result = false;
	entprops->GetEntPropBool(GetIndex(), Prop_Send, "m_bBuilding", result);
	return result;
}

bool tfentities::HBaseObject::IsBeingCarried() const
{
	bool result = false;
	entprops->GetEntPropBool(GetIndex(), Prop_Send, "m_bCarried", result);
	return result;
}

bool tfentities::HBaseObject::IsSapped() const
{
	bool result = false;
	entprops->GetEntPropBool(GetIndex(), Prop_Send, "m_bHasSapper", result);
	return result;
}

bool tfentities::HBaseObject::IsRedeploying() const
{
	bool result = false;
	entprops->GetEntPropBool(GetIndex(), Prop_Send, "m_bCarryDeploy", result);
	return result;
}

bool tfentities::HBaseObject::IsMiniBuilding() const
{
	bool result = false;
	entprops->GetEntPropBool(GetIndex(), Prop_Send, "m_bMiniBuilding", result);
	return result;
}

bool tfentities::HBaseObject::IsDisposableBuilding() const
{
	bool result = false;
	entprops->GetEntPropBool(GetIndex(), Prop_Send, "m_bDisposableBuilding", result);
	return result;
}

int tfentities::HBaseObject::GetMaxUpgradeMetal() const
{
	int value = 0;
	entprops->GetEntProp(GetIndex(), Prop_Send, "m_iUpgradeMetalRequired", value);
	return value;
}

int tfentities::HBaseObject::GetUpgradeMetal() const
{
	int value = 0;
	entprops->GetEntProp(GetIndex(), Prop_Send, "m_iUpgradeMetal", value);
	return value;
}

int tfentities::HTeamControlPoint::GetGroup() const
{
	int value = 0;
	entprops->GetEntProp(GetIndex(), Prop_Data, "m_iCPGroup", value);
	return value;
}

int tfentities::HTeamControlPoint::GetPointIndex() const
{
	int value = 0;
	entprops->GetEntProp(GetIndex(), Prop_Data, "m_iPointIndex", value);
	return value;
}

TeamFortress2::TFTeam tfentities::HTeamControlPoint::GetDefaultOwner() const
{
	int value = 0;
	entprops->GetEntProp(GetIndex(), Prop_Data, "m_iDefaultOwner", value);
	return static_cast<TeamFortress2::TFTeam>(value);
}

bool tfentities::HTeamControlPoint::IsLocked() const
{
	int value = 0;
	entprops->GetEntProp(GetIndex(), Prop_Data, "m_bLocked", value);
	return value != 0;
}

std::string tfentities::HTeamControlPoint::GetPrintName() const
{
	constexpr auto size = 512;
	char result[size]{};
	std::string ret;
	size_t len = 0;

	if (entprops->GetEntPropString(GetIndex(), Prop_Data, "m_iszPrintName", result, size, len))
	{
		ret.assign(result);
		return ret;
	}

	return ret;
}

tfentities::HObjectTeleporter::HObjectTeleporter(edict_t* entity) :
	HBaseObject(entity)
{
}

tfentities::HObjectTeleporter::HObjectTeleporter(CBaseEntity* entity) :
	HBaseObject(entity)
{
}

TeamFortress2::TeleporterState tfentities::HObjectTeleporter::GetState() const
{
	int state = 0;
	entprops->GetEntProp(GetIndex(), Prop_Send, "m_iState", state);
	return static_cast<TeamFortress2::TeleporterState>(state);
}
