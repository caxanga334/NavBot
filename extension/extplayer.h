#ifndef EXT_PLAYER_INTERFACE_H_
#define EXT_PLAYER_INTERFACE_H_

class CNavArea;
class CBaseBot;
class CTakeDamageInfo;

#include <iplayerinfo.h>

// Base player class, all code shared by players and bots goes here
class CBaseExtPlayer
{
public:
	CBaseExtPlayer(edict_t* edict);
	virtual ~CBaseExtPlayer();

	static bool InitHooks(SourceMod::IGameConfig* gd_navbot, SourceMod::IGameConfig* gd_sdkhooks, SourceMod::IGameConfig* gd_sdktools);

	/**
	 * @brief Radius the player is able to use objects.
	 * https://github.com/ValveSoftware/source-sdk-2013/blob/39f6dde8fbc238727c020d13b05ecadd31bda4c0/src/game/shared/baseplayer_shared.h#L15
	 */
	static constexpr float PLAYER_USE_RADIUS = 80.0f;

	// Gets the player edict_t
	inline edict_t* GetEdict() const { return m_edict; }
	// Gets the player entity index
	inline int GetIndex() const { return m_index; }
	// Gets the player CBaseEntity pointer
	CBaseEntity* GetEntity() const { return m_pEntity; }
	IServerEntity* GetServerEntity() const { return m_edict->GetIServerEntity(); }
	IServerUnknown* GetServerUnknown() const { return m_edict->GetUnknown(); }
	IHandleEntity* GetHandleEntity() const { return reinterpret_cast<IHandleEntity*>(m_edict->GetIServerEntity()); }
	IServerNetworkable* GetServerNetworkable() const { return m_edict->GetNetworkable(); }
	ICollideable* GetCollideable() const { return m_edict->GetCollideable(); }
	bool IsFakeClient() const { return m_playerinfo->IsFakeClient(); }

	bool operator==(const CBaseExtPlayer& other) const;

	/**
	 * @brief Called post constructor while still inside the OnClientPutInServer callback.
	 */
	virtual void PostConstruct();

	// Function called every server frame by the manager
	virtual void PlayerThink();

	/**
	 * @brief Called when the nearest nav area to the player changes. Areas can be NULL
	 * @param old Old nav area
	 * @param current New nav area
	*/
	inline virtual void NavAreaChanged(CNavArea* old, CNavArea* current) {}
	inline CNavArea* GetLastKnownNavArea() const { return m_lastnavarea; }
	void SetLastKnownNavArea(CNavArea* area) { if (area) { m_lastnavarea = area; } }
	void UpdateLastKnownNavArea(const bool forceupdate = false);
	inline void ClearLastKnownNavArea() { m_lastnavarea = nullptr; }
	inline IPlayerInfo* GetPlayerInfo() { return m_playerinfo; }
	const Vector WorldSpaceCenter() const;
	const Vector GetAbsOrigin() const;
	const QAngle GetAbsAngles() const;
	const Vector GetEyeOrigin() const;
	const QAngle& GetEyeAngles() const;
	const QAngle& GetLocalEyeAngles() const;
	const Vector GetMins() const;
	const Vector GetMaxs() const;
	const QAngle GetPunchAngle() const;
	CBaseEntity* GetMoveParent() const;
	/**
	 * @brief Utility function for finding a position to aim at for headshots.
	 * @param bonename Name of the head bone.
	 * @param result Vector to store the result at.
	 */
	void GetHeadShotPosition(const char* bonename, Vector& result) const;
	void EyeVectors(Vector* pForward) const;
	void EyeVectors(Vector* pForward, Vector* pRight, Vector* pUp) const;
	const Vector GetAbsVelocity() const;
	void SetAbsVelocity(const Vector& velocity) const;
	inline QAngle BodyAngles() const { return GetAbsAngles(); }
	Vector BodyDirection3D() const;
	Vector BodyDirection2D() const;
	// Gets the player model scale, virtual in case a mod uses different scaling
	virtual float GetModelScale() const;
	void Teleport(const Vector& origin, QAngle* angles = nullptr, Vector* velocity = nullptr) const;
	// Changes the player team
	virtual void ChangeTeam(int newTeam);
	virtual int GetCurrentTeamIndex() const;
	// true if this is a bot managed by this extension
	inline virtual bool IsExtensionBot() const { return false; }
	// Pointer to the extension bot class
	inline virtual CBaseBot* MyBotPointer() { return nullptr; }

	virtual MoveType_t GetMoveType() const;
	virtual CBaseEntity* GetGroundEntity() const;
	virtual bool Weapon_OwnsThisType(const char* weapon, edict_t** result = nullptr);
	// Gets the current active weapon or NULL if none
	virtual CBaseEntity* GetActiveWeapon() const;
	// True if the current active weapon matches the given classname
	virtual bool IsActiveWeapon(const char* classname) const;
	virtual int GetHealth() const { return m_playerinfo->GetHealth(); }
	virtual int GetMaxHealth() const { return m_playerinfo->GetMaxHealth(); }
	inline float GetHealthPercentage() const { return static_cast<float>(GetHealth()) / static_cast<float>(GetMaxHealth()); }
	int GetAmmoOfIndex(int index) const;

	std::vector<edict_t*> GetAllWeapons() const;

	virtual float GetMaxSpeed() const;
	inline bool IsOnLadder() const { return GetMoveType() == MOVETYPE_LADDER; }
	bool IsAlive() const;

	/**
	 * @brief Switch weapons by calling the game's function
	 * @param weapon Weapon entity
	 * @return true if the switch was made.
	 */
	bool SelectWeapon(CBaseEntity* weapon) const;
	/**
	 * @brief Given a weapon slot, returns a pointer to a weapon occupying the slot.
	 * @param slot Weapon slot to get weapon from.
	 * @return A CBaseEntity pointer of the weapon or NULL if no weapon is found.
	 */
	CBaseEntity* GetWeaponOfSlot(int slot) const;
	/**
	 * @brief Sets the player view angles. Reimplementation of the SDK's CBasePlayer::SnapEyeAngles
	 * @param viewAngles Angles to set.
	 */
	void SnapEyeAngles(const QAngle& viewAngles) const;

	static constexpr auto DEFAULT_LOOK_TOWARDS_TOLERANCE = 0.9f;

	/**
	 * @brief Checks if the player is looking towars the given entity.
	 * @param edict Entity to check if the player is looking at.
	 * @param tolerance Dot product tolerance.
	 * @return true if looking, false otherwise.
	 */
	bool IsLookingTowards(edict_t* edict, const float tolerance = DEFAULT_LOOK_TOWARDS_TOLERANCE) const;
	/**
	 * @brief Checks if the player is looking towars the given entity.
	 * @param entity Entity to check if the player is looking at.
	 * @param tolerance Dot product tolerance.
	 * @return true if looking, false otherwise.
	 */
	bool IsLookingTowards(CBaseEntity* entity, const float tolerance = DEFAULT_LOOK_TOWARDS_TOLERANCE) const;
	/**
	 * @brief Checks if the player is looking towars the given position.
	 * @param position Position to check if the player is looking at.
	 * @param tolerance Dot product tolerance.
	 * @return true if looking, false otherwise.
	 */
	bool IsLookingTowards(const Vector& position, const float tolerance = DEFAULT_LOOK_TOWARDS_TOLERANCE) const;

	// returns the player's water level
	int GetWaterLevel() const;
	// is the player underwater?
	bool IsUnderWater() const;
	/**
	 * @brief Checks if the player is moving towards the given position by comparing their velocity.
	 * @param position 
	 * @param tolerance Dot product result tolerance to consider true (between -1.0 and 1.0). 0.0 means 180 degrees.
	 * @param distance Optional float to store the distance between the player and the given position.
	 * @return true if the player is moving towars the given position within the given tolerance. false otherwise.
	 */
	bool IsMovingTowards(const Vector& position, const float tolerance = 0.5f, float* distance = nullptr) const;
	/**
	 * @brief Checks if the player is moving towards the given position by comparing their velocity.
	 * 
	 * Ignores Z axis.
	 * @param position
	 * @param tolerance Dot product result tolerance to consider true (between -1.0 and 1.0). 0.0 means 180 degrees.
	 * @param distance Optional float to store the distance between the player and the given position.
	 * @return true if the player is moving towars the given position within the given tolerance. false otherwise.
	 */
	bool IsMovingTowards2D(const Vector& position, const float tolerance = 0.5f, float* distance = nullptr) const;
	/**
	 * @brief Calculates a velocity to launch the player towards the given landing position.
	 * @param landing Landing position.
	 * @param speed Speed to launch the player.
	 * @return Velocity that should be set on the player to land on the target.
	 */
	Vector CalculateLaunchVector(const Vector& landing, const float speed) const;
	// returns true if the player is touching an entity of the given classnamew
	bool IsTouching(const char* classname) const;
protected:

private:
	edict_t* m_edict; // my edict pointer
	CBaseEntity* m_pEntity; // my cbaseentity pointer
	CPlayerState* m_pl; // player state pointer (cached)
	int m_index; // Index of this player
	IPlayerInfo* m_playerinfo;
	CNavArea* m_lastnavarea;
	int m_navupdatetimer;
	int m_prethinkHook;

	void SetupPlayerHooks();
	/* CBasePlayer::PreThink hook */
	void Hook_PreThink();
};


#endif // !EXT_PLAYER_INTERFACE_H_
