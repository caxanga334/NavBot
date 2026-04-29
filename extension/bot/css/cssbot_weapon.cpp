#include NAVBOT_PCH_FILE
#include "cssbot.h"
#include "cssbot_weapon.h"

CCSSBotWeapon::CCSSBotWeapon(CBaseEntity* entity) :
	CBotWeapon(entity)
{
}

bool CCSSBotWeapon::IsDeployedOrScoped(const CBaseBot* owner) const
{
	int fov = 0;
	entprops->GetEntProp(owner->GetEntity(), Prop_Send, "m_iFOV", fov);

	// in CSS, m_iFOV value changes when scoped
	if (fov > 0 && fov < 70)
	{
		return true;
	}

	return false;
}
