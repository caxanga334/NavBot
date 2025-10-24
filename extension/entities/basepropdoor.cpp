#include NAVBOT_PCH_FILE
#include "basepropdoor.h"

sdkdefs::DoorState_t entities::HBasePropDoor::GetDoorState() const
{
	int doorstate = 0;
	entprops->GetEntProp(GetBaseEntity(), Prop_Data, "m_eDoorState", doorstate);
	return static_cast<sdkdefs::DoorState_t>(doorstate);
}

bool entities::HBasePropDoor::IsLocked() const
{
	bool locked = false;
	entprops->GetEntPropBool(GetBaseEntity(), Prop_Data, "m_bLocked", locked);
	return locked;
}

QAngle entities::HPropDoorRotating::GetRotationAngleClosed() const
{
	Vector vec{ 0.0f, 0.0f, 0.0f };
	entprops->GetEntPropVector(GetBaseEntity(), Prop_Data, "m_angRotationClosed", vec);
	return QAngle{ vec.x, vec.y, vec.z };
}

QAngle entities::HPropDoorRotating::GetRotationAngleOpenForward() const
{
	Vector vec{ 0.0f, 0.0f, 0.0f };
	entprops->GetEntPropVector(GetBaseEntity(), Prop_Data, "m_angRotationOpenForward", vec);
	return QAngle{ vec.x, vec.y, vec.z };
}

QAngle entities::HPropDoorRotating::GetRotationAngleOpenBackwards() const
{
	Vector vec{ 0.0f, 0.0f, 0.0f };
	entprops->GetEntPropVector(GetBaseEntity(), Prop_Data, "m_angRotationOpenBack", vec);
	return QAngle{ vec.x, vec.y, vec.z };
}
