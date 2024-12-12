#ifndef NAVBOT_UTILS_SDKCALL_HELPER_H_
#define NAVBOT_UTILS_SDKCALL_HELPER_H_

#include <IBinTools.h>

class CBaseEntity;
class CGameRules;

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
	 * @param pBCC A pointer to a CBaseCombatCharacter entity.
	 * @param slot Weapon slot.
	 * @return Entity to a weapon at the given weapon slot. NULL if none.
	 */
	CBaseEntity* CBaseCombatCharacter_Weapon_GetSlot(CBaseEntity* pBCC, int slot);

	/**
	 * @brief Calls the game's CGameRules::ShouldCollide function.
	 * @param pGR Pointer to the gamerules object
	 * @param collisionGroup0 First collision group
	 * @param collisionGroup1 Second collision group
	 * @return true if the given groups should collide, false otherwise
	 */
	bool CGameRules_ShouldCollide(CGameRules* pGR, int collisionGroup0, int collisionGroup1);

	/**
	 * @brief Gets an entity eye position via SDK Call.
	 * @param entity Entity to get the eye position from.
	 * @return Eye position.
	 */
	Vector CBaseEntity_EyePosition(CBaseEntity* entity);

	/**
	 * @brief Gets an entity eye angles via SDK Call.
	 * @param entity Entity to get the eye angles from.
	 * @return Eye angles.
	 */
	QAngle CBaseEntity_EyeAngles(CBaseEntity* entity);

private:
	static constexpr int invalid_offset() { return -1; }

	bool m_init;

	//  CBaseCombatCharacter::Weapon_Switch
	int m_offsetof_cbc_weaponswitch;
	SourceMod::ICallWrapper* m_call_cbc_weaponswitch;

	//  CBaseCombatCharacter::Weapon_GetSlot
	int m_offsetof_cbc_weaponslot;
	SourceMod::ICallWrapper* m_call_cbc_weaponslot;

	//  CGameRules::ShouldCollide(int, int)
	int m_offsetof_cgr_shouldcollide;
	SourceMod::ICallWrapper* m_call_cgr_shouldcollide;

	// Vector CBaseEntity::EyePosition()
	int m_offsetof_cbe_eyeposition;
	SourceMod::ICallWrapper* m_call_cbe_eyeposition;

	// QAngle CBaseEntity::EyeAngles()
	int m_offsetof_cbe_eyeangles;
	SourceMod::ICallWrapper* m_call_cbe_eyeangles;

	bool SetupCalls();
	void SetupCBCWeaponSwitch();
	void SetupCBCWeaponSlot();
	void SetupCGRShouldCollide();
	void SetupCBEEyePosition();
	void SetupCBEEyeAngles();
};

extern CSDKCaller* sdkcalls;

#endif // !NAVBOT_UTILS_SDKCALL_HELPER_H_
