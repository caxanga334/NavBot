#ifndef __NAVBOT_NAV_MESH_BLOCKER_DOOR_H_
#define __NAVBOT_NAV_MESH_BLOCKER_DOOR_H_

#include "nav_blocker.h"

class CDoorNavBlocker : public CNavBlocker<CNavArea>
{
public:
	CDoorNavBlocker();

	enum DoorType
	{
		DOORTYPE_BRUSH, // brush based doors
		DOORTYPE_PROP // prop_door_rotating door
	};

	enum DoorActivator
	{
		ACTIVATOR_NONE, // nothing is connected to this door
		ACTIVATOR_TRIGGER, // door is controlled by a trigger brush entity
		ACTIVATOR_BUTTON, // door is controlled by a button
	};

	void Init(CBaseEntity* door)
	{
		m_door = door;
		UpdateDoor();
	}

	bool IsValid() override;
	void Update() override;
	bool IsBlocked(int teamID) override;
	bool RemoveOnRecompute() override { return true; }

protected:
	DoorType m_doortype;
	DoorActivator m_activatortype;
	CHandle<CBaseEntity> m_door;
	CHandle<CBaseEntity> m_trigger;
	CHandle<CBaseEntity> m_button;
	CHandle<CBaseEntity> m_filter; // filter entity if any
	bool m_blocked; // blocked status
	bool m_teamOnly; // Is the door team specific (if true, is always blocked for teams that doesn't match the door's team)
	int m_teamNum; // Team the door is assigned to

	virtual void UpdateDoor();
};


#endif // __NAVBOT_NAV_MESH_BLOCKER_DOOR_H_