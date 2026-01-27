#ifndef __NAVBOT_INSMICBOT_WEAPONS_INTERFACE_H_
#define __NAVBOT_INSMICBOT_WEAPONS_INTERFACE_H_

#include <bot/interfaces/weapon.h>

class CInsMICBotWeapon : public CBotWeapon
{
public:
	CInsMICBotWeapon(CBaseEntity* entity);

	int GetPrimaryAmmoLeft(const CBaseBot* owner) const override;
	int GetSecondaryAmmoLeft(const CBaseBot* owner) const override;
	bool IsLoaded() const override;
	bool IsDeployedOrScoped(const CBaseBot* owner) const override;
	int GetSubType() const override { return 0; /* subtype doesn't exists in Insurgency */ }
	bool HasGrenadeLauncher() const { return m_hasGL; }

private:

	enum WeaponType
	{
		TYPE_CLIP = 0,
		TYPE_LOADABLE,
	};

	WeaponType m_weaponType;
	bool m_hasGL; // does this weapon have a grenade launcher?

	void DetermineWeaponType(CBaseEntity* entity);
};


#endif // !__NAVBOT_INSMICBOT_WEAPONS_INTERFACE_H_