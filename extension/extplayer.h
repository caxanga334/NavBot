#ifndef EXT_PLAYER_INTERFACE_H_
#define EXT_PLAYER_INTERFACE_H_

#include <vector>

class CNavArea;
class CBaseBot;
class CTakeDamageInfo;

#include <eiface.h>
#include <iplayerinfo.h>
#include <const.h>

// Base player class, all code shared by players and bots goes here
class CBaseExtPlayer
{
public:
	CBaseExtPlayer(edict_t* edict);
	virtual ~CBaseExtPlayer();

	// Gets the player edict_t
	inline edict_t* GetEdict() const { return m_edict; }
	// Gets the player entity index
	inline int GetIndex() const { return m_index; }
	CBaseEntity* GetEntity() const;
	IServerEntity* GetServerEntity() const { return m_edict->GetIServerEntity(); }
	IServerUnknown* GetServerUnknown() const { return m_edict->GetUnknown(); }
	IHandleEntity* GetHandleEntity() const { return reinterpret_cast<IHandleEntity*>(m_edict->GetIServerEntity()); }
	IServerNetworkable* GetServerNetworkable() const { return m_edict->GetNetworkable(); }
	ICollideable* GetCollideable() const { return m_edict->GetCollideable(); }

	bool operator==(const CBaseExtPlayer& other) const;

	// Function called every server frame by the manager
	virtual void PlayerThink();

	/**
	 * @brief Called when the nearest nav area to the player changes. Areas can be NULL
	 * @param old Old nav area
	 * @param current New nav area
	*/
	inline virtual void NavAreaChanged(CNavArea* old, CNavArea* current) {}
	inline CNavArea* GetLastKnownNavArea() const { return m_lastnavarea; }
	void UpdateLastKnownNavArea(const bool forceupdate = false);
	inline void ClearLastKnownNavArea() { m_lastnavarea = nullptr; }
	inline IPlayerInfo* GetPlayerInfo() { return m_playerinfo; }
	const Vector WorldSpaceCenter() const;
	const Vector GetAbsOrigin() const;
	const QAngle GetAbsAngles() const;
	const Vector GetEyeOrigin() const;
	const QAngle GetEyeAngles() const;
	const Vector GetMins() const;
	const Vector GetMaxs() const;
	void EyeVectors(Vector* pForward) const;
	void EyeVectors(Vector* pForward, Vector* pRight, Vector* pUp) const;
	const Vector GetAbsVelocity() const;
	void SetAbsVelocity(const Vector& velocity) const;
	inline QAngle BodyAngles() const { return GetAbsAngles(); }
	Vector BodyDirection3D() const;
	Vector BodyDirection2D() const;
	// Gets the player model scale, virtual in case a mod uses different scaling
	virtual float GetModelScale() const;
	// Changes the player team
	virtual void ChangeTeam(int newTeam);
	virtual int GetCurrentTeamIndex() const;
	// true if this is a bot managed by this extension
	inline virtual bool IsExtensionBot() const { return false; }
	// Pointer to the extension bot class
	inline virtual CBaseBot* MyBotPointer() { return nullptr; }

	virtual MoveType_t GetMoveType();
	virtual edict_t* GetGroundEntity();
	virtual bool Weapon_OwnsThisType(const char* weapon, edict_t** result = nullptr);
	// Gets the current active weapon or NULL if none
	virtual edict_t* GetActiveWeapon() const;
	// True if the current active weapon matches the given classname
	virtual bool IsActiveWeapon(const char* classname) const;
	virtual int GetHealth() const { return m_playerinfo->GetHealth(); }
	virtual int GetMaxHealth() const { return m_playerinfo->GetMaxHealth(); }
	inline float GetHealthPercentage() const { return static_cast<float>(GetHealth()) / static_cast<float>(GetMaxHealth()); }
	int GetAmmoOfIndex(int index) const;

	std::vector<edict_t*> GetAllWeapons() const;

	virtual float GetMaxSpeed() const;

protected:

private:
	edict_t* m_edict;
	int m_index; // Index of this player
	IPlayerInfo* m_playerinfo;
	CNavArea* m_lastnavarea;
	int m_navupdatetimer;
};


#endif // !EXT_PLAYER_INTERFACE_H_
