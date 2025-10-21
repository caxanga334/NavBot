#ifndef NAVBOT_ENTITIES_BASE_COMBAT_WEAPON_H_
#define NAVBOT_ENTITIES_BASE_COMBAT_WEAPON_H_

#include <entities/baseentity.h>

struct edict_t;

namespace entities
{
	class HBaseCombatWeapon : public HBaseEntity
	{
	public:
		HBaseCombatWeapon(edict_t* entity);
		HBaseCombatWeapon(CBaseEntity* entity);

		int GetClip1() const;
		int GetClip2() const;
		bool UsesClipsForAmmo1() const;
		bool UsesClipsForAmmo2() const;
		int GetPrimaryAmmoType() const;
		int GetSecondaryAmmoType() const;
		int GetPrimaryAmmoCount() const;
		int GetSecondaryAmmoCount() const;
		int GetState() const;
		int GetOwnerIndex() const;
		edict_t* GetOwner() const;
		bool IsOnGround() const;
		bool IsCarried() const;
		bool IsActive() const;
		// Reads the m_iSubType property, this is where the weapon subtype is stored in the base class but since the subtype function is virtual, 
		// this may not be the correct weapon subtype
		int GetSubTypeFromProperty() const;
	};
}


#endif // !NAVBOT_ENTITIES_BASE_COMBAT_WEAPON_H_

