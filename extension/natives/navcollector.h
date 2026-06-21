#ifndef __NAVBOT_NATIVES_NAV_COLLECTOR_H_
#define __NAVBOT_NATIVES_NAV_COLLECTOR_H_

namespace natives::navmesh::collector
{
	void setup(std::vector<sp_nativeinfo_t>& nv);
	void destroy(void* object);
}

#endif // !__NAVBOT_NATIVES_NAV_COLLECTOR_H_
