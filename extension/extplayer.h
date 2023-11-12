#ifndef EXT_PLAYER_INTERFACE_H_
#define EXT_PLAYER_INTERFACE_H_

#include <eiface.h>
#include <iplayerinfo.h>
#include "navmesh/nav_area.h"

class CBaseBot;

// Base interface for players and bots
class CBaseExtPlayer
{
public:
	CBaseExtPlayer(edict_t* edict);
	virtual ~CBaseExtPlayer();

	virtual edict_t* GetEdict() { return m_edict; }

	// Function called every server frame by the manager
	virtual void PlayerThink();

	/**
	 * @brief Called when the nearest nav area to the player changes. Areas can be NULL
	 * @param old Old nav area
	 * @param current New nav area
	*/
	inline virtual void OnNavAreaChanged(CNavArea* old, CNavArea* current) {}
	inline virtual CNavArea* GetLastKnownNavArea() { return m_lastnavarea; }
	inline virtual IPlayerInfo* GetPlayerInfo() { return m_playerinfo; }
	const Vector GetAbsOrigin();
	const QAngle GetAbsAngles();
	const Vector GetEyeOrigin();
	const QAngle GetEyeAngles();
	void EyeVectors(Vector* pForward);
	void EyeVectors(Vector* pForward, Vector* pRight, Vector* pUp);
	inline QAngle BodyAngles() { return GetAbsAngles(); }
	Vector BodyDirection3D();
	Vector BodyDirection2D();
	// true if this is a bot managed by this extension
	inline virtual bool IsExtensionBot() { return false; }
	// Pointer to the extension bot class
	inline virtual CBaseBot* MyBotPointer() { return nullptr; }

protected:

private:
	edict_t* m_edict;
	IPlayerInfo* m_playerinfo;
	CNavArea* m_lastnavarea;
	int m_navupdatetimer;
};

inline CBaseExtPlayer::CBaseExtPlayer(edict_t* edict)
{
	m_edict = edict;
	m_playerinfo = nullptr;
	m_lastnavarea = nullptr;
	m_navupdatetimer = 64;
}

inline CBaseExtPlayer::~CBaseExtPlayer()
{
}


#endif // !EXT_PLAYER_INTERFACE_H_
