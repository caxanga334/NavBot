#include NAVBOT_PCH_FILE
#include "sdk_doors.h"
#include "sdk_subs.h"

namespace sdk::subs::basetoggle
{
	float AxisDelta(int flags, const QAngle& angle1, const QAngle& angle2)
	{
		// UNDONE: Use AngleDistance() here?
		if (sdk::IsBitSet(flags, sdk::doors::SF_DOOR_ROTATE_ROLL))
			return angle1.z - angle2.z;

		if (sdk::IsBitSet(flags, sdk::doors::SF_DOOR_ROTATE_PITCH))
			return angle1.x - angle2.x;

		return angle1.y - angle2.y;
	}
}

namespace sdk::buttons
{
	float ComputeMomentaryRotButtonPos(CBaseEntity* entity)
	{
		// all calls to CMomentaryRotButton::GetPos passes GetLocalAngles
		const QAngle& vecAngles = entityprops::GetEntityLocalAngles(entity);
		QAngle vecMoveAng(0.0f, 0.0f, 0.0f);
		entprops->GetEntPropAngles(entity, Prop_Data, "m_vecMoveAng", vecMoveAng);

		float flScale = 1;
		if ((vecMoveAng[0] < 0) || (vecMoveAng[1] < 0) || (vecMoveAng[2] < 0))
		{
			flScale = -1;
		}

		float flMoveDistance = 0.0f;
		entprops->GetEntPropFloat(entity, Prop_Data, "m_flMoveDistance", flMoveDistance);
		QAngle start;
		entprops->GetEntPropAngles(entity, Prop_Data, "m_start", start);
		int spawnflags = 0;
		entprops->GetEntProp(entity, Prop_Data, "m_spawnflags", spawnflags);

		float flPos = flScale * sdk::subs::basetoggle::AxisDelta(spawnflags, vecAngles, start) / flMoveDistance;
		return(std::clamp(flPos, 0.0f, 1.0f));
	}

	bool IsMomentaryRotButton(CBaseEntity* entity)
	{
		// On two different games, m_returnSpeed is exclusive to CMomentaryRotButton
		// This method is preferred over classname checks since it detects derived classes and custom entity classnames.
		return entprops->HasEntProp(entity, Prop_Data, "m_returnSpeed");
	}
}