#ifndef __NAVBOT_DODS_NAV_AREA_H_
#define __NAVBOT_DODS_NAV_AREA_H_

#include <navmesh/nav_area.h>

class CDODSNavArea : public CNavArea
{
public:
	CDODSNavArea(unsigned int place);
	~CDODSNavArea() override;


private:

};

#endif // !__NAVBOT_DODS_NAV_AREA_H_
