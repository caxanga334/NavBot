#ifndef __NAVBOT_NATIVES_NAV_AREA_VECTOR_H_
#define __NAVBOT_NATIVES_NAV_AREA_VECTOR_H_

#include <ISourceMod.h>

namespace natives::navmesh::navareavector
{
	void setup(std::vector<sp_nativeinfo_t>& nv);
	SourceMod::Handle_t createhandle(SourcePawn::IPluginContext* context, std::vector<CNavArea*>** object);
	std::vector<CNavArea*>* readhandle(SourcePawn::IPluginContext* context, SourceMod::Handle_t handle);
}

#endif // !__NAVBOT_NATIVES_NAV_AREA_VECTOR_H_
