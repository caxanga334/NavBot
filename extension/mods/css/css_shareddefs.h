#ifndef __NAVBOT_CSS_SHAREDDEFS_H_
#define __NAVBOT_CSS_SHAREDDEFS_H_

namespace counterstrikesource
{
	enum CSSTeam
	{
		UNASSIGNED = 0,
		SPECTATOR,
		TERRORIST,
		COUNTERTERRORIST,

		MAX_CSS_TEAMS
	};

	constexpr int CSS_WEAPON_SLOT_PRIMARY = 0;
	constexpr int CSS_WEAPON_SLOT_SECONDARY = 1;
	constexpr int CSS_WEAPON_SLOT_KNIFE = 2;
	constexpr int CSS_WEAPON_SLOT_GRENADE = 3;
	constexpr int CSS_WEAPON_SLOT_C4 = 4;
}


#endif // !__NAVBOT_CSS_SHAREDDEFS_H_

