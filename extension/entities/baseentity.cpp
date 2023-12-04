#include <extension.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <entities/baseentity.h>

entities::HBaseEntity::HBaseEntity(edict_t* entity)
{
	m_index = gamehelpers->IndexOfEdict(entity);
}

entities::HBaseEntity::HBaseEntity(CBaseEntity* entity)
{
	m_index = gamehelpers->EntityToBCompatRef(entity);
}

/**
 * @brief Gets the actual entity as a CBaseEntity or edict
 * @return false if lookup failed
*/
bool entities::HBaseEntity::GetEntity(CBaseEntity** entity, edict_t** edict) const
{
	return UtilHelpers::IndexToAThings(GetIndex(), entity, edict);
}

Vector entities::HBaseEntity::GetAbsOrigin() const
{
	Vector result;

	if (entprops->GetEntPropVector(GetIndex(), Prop_Data, "m_vecOrigin", result) == true)
	{
		return result;
	}

	return vec3_origin;
}

QAngle entities::HBaseEntity::GetAbsAngles() const
{
	Vector result;

	if (entprops->GetEntPropVector(GetIndex(), Prop_Data, "m_angRotation", result) == true)
	{
		return QAngle(result.x, result.y, result.z);
	}

	return vec3_angle;
}