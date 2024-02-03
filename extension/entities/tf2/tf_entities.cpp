#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <mods/tf2/tf2lib.h>
#include "tf_entities.h"

tfentities::HCaptureFlag::HCaptureFlag(edict_t* entity) : entities::HBaseEntity(entity)
{
}

tfentities::HCaptureFlag::HCaptureFlag(CBaseEntity* entity) : entities::HBaseEntity(entity)
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

TeamFortress2::TFTeam tfentities::HCaptureFlag::GetTFTeam() const
{
	return tf2lib::GetEntityTFTeam(GetIndex());
}

tfentities::HCaptureZone::HCaptureZone(edict_t* entity) : entities::HBaseEntity(entity)
{
}

tfentities::HCaptureZone::HCaptureZone(CBaseEntity* entity) : entities::HBaseEntity(entity)
{
}

bool tfentities::HCaptureZone::IsDisabled() const
{
	bool result = false;
	entprops->GetEntPropBool(GetIndex(), Prop_Send, "m_bDisabled", result);
	return result;
}

TeamFortress2::TFTeam tfentities::HCaptureZone::GetTFTeam() const
{
	return tf2lib::GetEntityTFTeam(GetIndex());
}
