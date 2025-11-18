#ifndef __NAVBOT_TFNAV_DOOR_BLOCKER_H_
#define __NAVBOT_TFNAV_DOOR_BLOCKER_H_

#include <navmesh/nav_blocker_door.h>

class CTFDoorNavBlocker : public CDoorNavBlocker
{
public:

protected:
	void UpdateDoor() override;
};


#endif // !__NAVBOT_TFNAV_DOOR_BLOCKER_H_
