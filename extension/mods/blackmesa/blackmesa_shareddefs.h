#ifndef NAVBOT_BLACK_MESA_SHARED_DEFS_H_
#define NAVBOT_BLACK_MESA_SHARED_DEFS_H_

namespace blackmesa
{
	enum BMTeam : int
	{
		Team_Unassigned = 0,
		Team_Spectator,
		Team_Marines,
		Team_Scientists
	};
	
	/**
	 * @brief Indexes of each ammo type for CBaseCombatCharacter::m_iAmmo
	 */
	enum BMAmmoIndex : int
	{
		Ammo_9mm = 1, // 9mm ammo (glock, mp5)
		Ammo_357 = 2, // 357 magnum ammo (revolver)
		Ammo_Bolts = 3, // crossbow bolts (crossbow)
		Ammo_12ga = 4, // 12 gauge buckshot (spas)
		Ammo_DepletedUranium = 5, // Depleted uranium (tau, gluon)
		Ammo_40mmGL = 6, // 40mm grenades (mp5 alt fire)
		Ammo_Rockets = 7, // rockets (RPG)
		Ammo_FragGrenades = 8, // fragmentation grenades
		Ammo_Satchel = 9, // satchel charges
		Ammo_Tripmines = 10, // Tripmines
		Ammo_Hornets = 11, // Hornets (hivehand)
		Ammo_Snarks = 12, // Snarks
	};
}

#endif
