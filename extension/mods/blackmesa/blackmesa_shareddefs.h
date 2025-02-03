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
		Ammo_Invalid = -1, // Invalid ammo index

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
	
	/**
	 * @brief Given an ammo index, return the classname of the item entity that provides this ammo type.
	 * @param index Ammo index.
	 * @return Entity classname that gives this ammo type. NULL on invalid or none.
	 */
	inline const char* GetItemNameForAmmoIndex(BMAmmoIndex index)
	{
		switch (index)
		{
		case blackmesa::Ammo_9mm:
			return "item_ammo_mp5"; // item_ammo_glock
		case blackmesa::Ammo_357:
			return "item_ammo_357";
		case blackmesa::Ammo_Bolts:
			return "item_ammo_crossbow";
		case blackmesa::Ammo_12ga:
			return "item_ammo_shotgun";
		case blackmesa::Ammo_DepletedUranium:
			return "item_ammo_energy";
		case blackmesa::Ammo_40mmGL:
			return "item_grenade_mp5";
		case blackmesa::Ammo_Rockets:
			return "item_grenade_rpg";
		case blackmesa::Ammo_FragGrenades:
			return "item_weapon_frag";
		case blackmesa::Ammo_Satchel:
			return "item_weapon_satchel";
		case blackmesa::Ammo_Tripmines:
			return "item_weapon_tripmine";
		case blackmesa::Ammo_Hornets:
			return nullptr;
		case blackmesa::Ammo_Snarks:
			return "item_weapon_snark";
		default:
			return nullptr;
		}
	}
}

#endif
