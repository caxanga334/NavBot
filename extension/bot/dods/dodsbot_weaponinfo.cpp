#include NAVBOT_PCH_FILE
#include "dodsbot.h"
#include "dodsbot_weaponinfo.h"

CDoDSBotWeapon::CDoDSBotWeapon(CBaseEntity* entity) :
	CBotWeapon(entity)
{
}

bool CDoDSBotWeapon::IsMachineGunDeployed() const
{
	bool state = false;

	if (entprops->GetEntPropBool(GetEntity(), Prop_Send, "m_bDeployed", state))
	{
		return state;
	}

	return false;
}

bool CDoDSBotWeapon::IsScopedIn() const
{
	bool state = false;

	if (entprops->GetEntPropBool(GetEntity(), Prop_Send, "m_bZoomed", state))
	{
		return state;
	}

	return false;
}
