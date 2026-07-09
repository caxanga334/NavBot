#include NAVBOT_PCH_FILE
#include "zpsbot_weaponinfo.h"
#include "zpsbot_weapon.h"

CZPSBotWeapon::CZPSBotWeapon(CBaseEntity* entity) :
	CBotWeapon(entity)
{
}

const ZPSWeaponInfo* CZPSBotWeapon::GetZPSWeaponInfo() const
{
	return static_cast<const ZPSWeaponInfo*>(GetWeaponInfo());
}
