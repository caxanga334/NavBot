#include NAVBOT_PCH_FILE
#include <mods/zps/zps_lib.h>
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

bool CZPSBotWeapon::IsItemGenericOfID(const std::string& id) const
{
	if (!IsWeapon("item_deliver"))
	{
		return false;
	}

	std::string myid = zpslib::GetItemDeliverIDName(GetEntity());

	if (ke::StrCaseCmp(myid.c_str(), id.c_str()) == 0)
	{
		return true;
	}

	return false;
}
