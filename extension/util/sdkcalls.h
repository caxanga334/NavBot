#ifndef NAVBOT_UTILS_SDKCALL_HELPER_H_
#define NAVBOT_UTILS_SDKCALL_HELPER_H_

#include <IBinTools.h>

class CBaseEntity;
class CGameRules;
class CBotCmd;
class variant_t;

/**
 * @brief Class to manager SDK Calls
 */
class CSDKCaller
{
public:
	CSDKCaller();
	~CSDKCaller();

	/* Virtual calls: offset and callwrapper */
	using SDKVCallSetup = std::pair<int, SourceMod::ICallWrapper*>;

	constexpr static void InitVCallSetup(SDKVCallSetup& setup)
	{
		setup.first = invalid_offset();
		setup.second = nullptr;
	}

	bool Init();
	void PostInit();

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

	void CBasePlayer_ProcessUsercmds(CBaseEntity* pBP, CBotCmd* botcmd);

	void CBaseAnimating_GetBoneTransform(CBaseEntity* pBA, int bone, matrix3x4_t* result);
	/**
	 * @brief Calls CBaseEntity::AcceptInput on the pThis entity.
	 * @param pThis Entity to call the function.
	 * @param szInputName Input name.
	 * @param pActivator Input activator.
	 * @param pCaller Input caller.
	 * @param variant Variant_t to pass to the call.
	 * @param outputID Output ID
	 * @return True on success, false on failure.
	 */
	bool CBaseEntity_AcceptInput(CBaseEntity* pThis, const char* szInputName, CBaseEntity* pActivator, CBaseEntity* pCaller, variant_t variant, int outputID);

	/**
	 * @brief Calls CBaseEntity::Teleport on the pThis entity.
	 * @param pThis Entity to call the function.
	 * @param origin New position. NULL for no change.
	 * @param angles New Angles. NULL for no change.
	 * @param velocity New velocity. NULL for no change.
	 */
	void CBaseEntity_Teleport(CBaseEntity* pThis, const Vector* origin = nullptr, const QAngle* angles = nullptr, const Vector* velocity = nullptr);

	inline bool IsProcessUsercmdsAvailable() const { return m_offsetof_cbp_processusercmds > 0; }
	inline bool IsGetBoneTransformAvailable() const { return m_offsetof_cba_getbonetransform > 0; }
	inline bool IsAcceptInputAvailable() const { return m_call_cbe_acceptinput.first > 0; }
	inline bool IsTeleportAvailable() const { return m_call_cbe_teleport.first > 0; }

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

	// https://github.com/ValveSoftware/source-sdk-2013/blob/master/mp/src/game/server/player.h#L577
	//  void CBasePlayer::ProcessUsercmds( CUserCmd *cmds, int numcmds, int totalcmds, int dropped_packets, bool paused )
	int m_offsetof_cbp_processusercmds;
	SourceMod::ICallWrapper* m_call_cbp_processusercmds;

	// https://github.com/ValveSoftware/source-sdk-2013/blob/master/mp/src/game/server/baseanimating.h#L136
	//  void CBaseAnimating::GetBoneTransform( int iBone, matrix3x4_t &pBoneToWorld )
	int m_offsetof_cba_getbonetransform;
	SourceMod::ICallWrapper* m_call_cba_getbonetransform;

	SDKVCallSetup m_call_cbe_acceptinput;
	SDKVCallSetup m_call_cbe_teleport;

	bool SetupCalls();
	void SetupCBCWeaponSwitch();
	void SetupCBCWeaponSlot();
	void SetupCGRShouldCollide();
	void SetupCBPProcessUserCmds();
	void SetupCBAGetBoneTransform();
	void SetupCBEAcceptInput();
	void SetupCBETeleport();
};

extern CSDKCaller* sdkcalls;

#endif // !NAVBOT_UTILS_SDKCALL_HELPER_H_
