#ifndef __NAVBOT_ZPSBOT_WEAPON_H_
#define __NAVBOT_ZPSBOT_WEAPON_H_

#include <bot/interfaces/weapon.h>

class ZPSWeaponInfo;

class CZPSBotWeapon : public CBotWeapon
{
public:
	CZPSBotWeapon(CBaseEntity* entity);

	const ZPSWeaponInfo* GetZPSWeaponInfo() const;
	bool IsItemGenericOfID(const std::string& id) const;
};

#endif // !__NAVBOT_ZPSBOT_WEAPON_H_
