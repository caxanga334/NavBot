#ifndef __NAVBOT_DODSBOT_WEAPONINFO_H_
#define __NAVBOT_DODSBOT_WEAPONINFO_H_

#include <bot/interfaces/weaponinfo.h>
#include <bot/interfaces/weapon.h>

class CDoDSBotWeapon : public CBotWeapon
{
public:
	CDoDSBotWeapon(CBaseEntity* entity);

	// Returns true if a machine gun is deployed.
	bool IsMachineGunDeployed() const;
	// Returns true if the weapon is scoped in/ADS
	bool IsScopedIn() const;

private:

};

#endif // !__NAVBOT_DODSBOT_WEAPONINFO_H_
