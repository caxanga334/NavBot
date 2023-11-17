#ifndef EXT_PLAYER_INTERFACE_H_
#define EXT_PLAYER_INTERFACE_H_

#include <eiface.h>
#include <iplayerinfo.h>
#include "navmesh/nav_area.h"

class CBaseBot;

// Base player class, all code shared by players and bots goes here
class CBaseExtPlayer
{
public:
	CBaseExtPlayer(edict_t* edict);
	virtual ~CBaseExtPlayer();

	// Gets the player edict_t
	inline edict_t* GetEdict() { return m_edict; }
	// Gets the player entity index
	inline int GetIndex() const { return m_index; }

	bool operator==(const CBaseExtPlayer& other);

	// Function called every server frame by the manager
	virtual void PlayerThink();

	/**
	 * @brief Called when the nearest nav area to the player changes. Areas can be NULL
	 * @param old Old nav area
	 * @param current New nav area
	*/
	inline virtual void OnNavAreaChanged(CNavArea* old, CNavArea* current) {}
	inline CNavArea* GetLastKnownNavArea() { return m_lastnavarea; }
	inline IPlayerInfo* GetPlayerInfo() { return m_playerinfo; }
	const Vector GetAbsOrigin();
	const QAngle GetAbsAngles();
	const Vector GetEyeOrigin();
	const QAngle GetEyeAngles();
	void EyeVectors(Vector* pForward);
	void EyeVectors(Vector* pForward, Vector* pRight, Vector* pUp);
	inline QAngle BodyAngles() { return GetAbsAngles(); }
	Vector BodyDirection3D();
	Vector BodyDirection2D();
	// Changes the player team
	virtual void ChangeTeam(int newTeam);
	virtual int GetCurrentTeamIndex();
	// true if this is a bot managed by this extension
	inline virtual bool IsExtensionBot() { return false; }
	// Pointer to the extension bot class
	inline virtual CBaseBot* MyBotPointer() { return nullptr; }

protected:

private:
	edict_t* m_edict;
	int m_index; // Index of this player
	IPlayerInfo* m_playerinfo;
	CNavArea* m_lastnavarea;
	int m_navupdatetimer;
};


#endif // !EXT_PLAYER_INTERFACE_H_
