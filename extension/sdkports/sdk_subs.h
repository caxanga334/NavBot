#ifndef __NAVBOT_SDKPORTS_SDK_SUBS_H_
#define __NAVBOT_SDKPORTS_SDK_SUBS_H_


namespace sdk
{
	// FBitSet replacement
	template <typename T>
	bool IsBitSet(T bitVec, T bits)
	{
		static_assert(std::is_integral_v<T>);

		if ((bitVec & bits) != 0)
		{
			return true;
		}

		return false;
	}
}

namespace sdk::subs::basetoggle
{
	float AxisDelta(int flags, const QAngle& angle1, const QAngle& angle2);
}

namespace sdk::buttons
{
	/**
	 * @brief Implementation of CMomentaryRotButton::GetPos
	 * @param entity Momentary rot button entity (derives from or is a CMomentaryRotButton)
	 * @return Returns the entity's angular position as a value along our motion from 0 to 1.
	 */
	float ComputeMomentaryRotButtonPos(CBaseEntity* entity);
	/**
	 * @brief Determines if the given entity is a momentary_rot_button (or derives from it).
	 * @param entity Entity pointer.
	 * @return True if the passed entity is or derives from momentary_rot_button, false otherwise.
	 */
	bool IsMomentaryRotButton(CBaseEntity* entity);
}

#endif // !__NAVBOT_SDKPORTS_SDK_SUBS_H_
