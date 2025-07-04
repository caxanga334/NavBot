#include NAVBOT_PCH_FILE
#include <extension.h>
#include <util/entprops.h>
#include "worldspawn.h"

Vector entities::HWorld::GetWorldMins() const
{
	Vector result;
	entprops->GetEntPropVector(GetIndex(), Prop_Data, "m_WorldMins", result);
	return result;
}

Vector entities::HWorld::GetWorldMaxs() const
{
	Vector result;
	entprops->GetEntPropVector(GetIndex(), Prop_Data, "m_WorldMaxs", result);
	return result;
}
