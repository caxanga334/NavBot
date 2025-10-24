#ifndef __NAVBOT_ENTITIES_BASEPROPDOOR_H_
#define __NAVBOT_ENTITIES_BASEPROPDOOR_H_

#include "props.h"

namespace entities
{
	class HBasePropDoor : public HDynamicProp
	{
	public:
		HBasePropDoor(edict_t* entity) : HDynamicProp(entity) {}
		HBasePropDoor(CBaseEntity* entity) : HDynamicProp(entity) {}

		sdkdefs::DoorState_t GetDoorState() const;
		bool IsLocked() const;
	};

	class HPropDoorRotating : public HBasePropDoor
	{
	public:
		HPropDoorRotating(edict_t* entity) : HBasePropDoor(entity) {}
		HPropDoorRotating(CBaseEntity* entity) : HBasePropDoor(entity) {}

		QAngle GetRotationAngleClosed() const;
		QAngle GetRotationAngleOpenForward() const;
		QAngle GetRotationAngleOpenBackwards() const;
	};
}


#endif // !__NAVBOT_ENTITIES_BASEPROPDOOR_H_
