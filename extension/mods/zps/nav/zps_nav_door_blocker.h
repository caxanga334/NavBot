#ifndef __NAVBOT_MOD_ZPS_NAV_DOOR_BLOCKER_H_
#define __NAVBOT_MOD_ZPS_NAV_DOOR_BLOCKER_H_

#include <navmesh/nav_blocker_door.h>

class CZPSDoorBlocker : public CDoorNavBlocker
{
public:
	bool IsValid() const override;
	void Update() override;

};

#endif // !__NAVBOT_MOD_ZPS_NAV_DOOR_BLOCKER_H_
