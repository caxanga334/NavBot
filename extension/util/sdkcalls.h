#ifndef NAVBOT_UTILS_SDKCALL_HELPER_H_
#define NAVBOT_UTILS_SDKCALL_HELPER_H_

#include <IBinTools.h>

class CBaseEntity;

/**
 * @brief Class to manager SDK Calls
 */
class CSDKCaller
{
public:
	CSDKCaller();
	~CSDKCaller();

	bool Init();

	/**
	 * @brief Calls the game's CBaseCombatCharacter::Weapon_Switch function.
	 * @param pBCC A pointer to a CBaseCombatCharacter entity.
	 * @param pWeapon A pointer to a CBaseCombatWeapon entity.
	 * @return true if the weapon switch was made.
	 */
	bool CBaseCombatCharacter_Weapon_Switch(CBaseEntity* pBCC, CBaseEntity* pWeapon);

	/**
	 * @brief Calls the game's CBaseCombatCharacter::Weapon_GetSlot function.
	 * @param slot Weapon slot.
	 * @return Entity to a weapon at the given weapon slot. NULL if none.
	 */
	CBaseEntity* CBaseCombatCharacter_Weapon_GetSlot(CBaseEntity* pBCC, int slot);

private:
	static constexpr int invalid_offset() { return -1; }

	bool m_init;

	//  CBaseCombatCharacter::Weapon_Switch
	int m_offsetof_cbc_weaponswitch;
	SourceMod::ICallWrapper* m_call_cbc_weaponswitch;

	//  CBaseCombatCharacter::Weapon_GetSlot
	int m_offsetof_cbc_weaponslot;
	SourceMod::ICallWrapper* m_call_cbc_weaponslot;

	bool SetupCalls();
	void SetupCBCWeaponSwitch();
	void SetupCBCWeaponSlot();
};

extern CSDKCaller* sdkcalls;

#endif // !NAVBOT_UTILS_SDKCALL_HELPER_H_
