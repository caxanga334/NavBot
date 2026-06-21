#ifndef __NAVBOT_NATIVES_NAVMESH_NAV_BLOCKER_H_
#define __NAVBOT_NATIVES_NAVMESH_NAV_BLOCKER_H_

namespace natives::navmesh::navblocker
{
	void setup(std::vector<sp_nativeinfo_t>& nv);
	void onhandledeleted(void* object);
}


#endif // !__NAVBOT_NATIVES_NAVMESH_NAV_BLOCKER_H_
