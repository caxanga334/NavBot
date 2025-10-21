#ifndef __NAVBOT_BOT_PATHCOSTS_H_
#define __NAVBOT_BOT_PATHCOSTS_H_
/*
* Collection of default path costs for bots.
*/

#include <array>
#include "interfaces/path/basepath.h"
#include <navmesh/nav_consts.h>

class IMovement;

/**
 * @brief Describes the movement capabilities of a human based player
 */
struct HumanMovementCaps_t
{
	HumanMovementCaps_t()
	{
		m_stepheight = 0.0f;
		m_maxjumpheight = 0.0f;
		m_maxdropheight = 0.0f;
		m_maxgapjumpdistance = 0.0f;
		m_maxdjheight = 0.0f;
		m_candoublejump = false;
	}

	void Init(IMovement* movement);

	float m_stepheight; // step height, if greater than this we have to jump
	float m_maxjumpheight; // maximum crouch jump height, if greater than this, we have to double jump or impossible path if DJ is not a thing.
	float m_maxdropheight; // maximum height we can drop, if greater than this, impossible path
	float m_maxgapjumpdistance; // maximum length of a gap jump, if greather than this, impossible path
	float m_maxdjheight; // maximum vertical height we can jump via double jump
	bool m_candoublejump; // can we double jump?
};

/**
 * @brief Common path cost for ground movement
 */
class IGroundPathCost : public IPathCost
{
public:
	IGroundPathCost() :
		m_movecaps()
	{
		m_moveiface = nullptr;
		m_ignoreDanger = false;
		m_teamindex = NAV_TEAM_ANY;
	}

	template <typename Bot>
	IGroundPathCost(Bot* bot)
	{
		IMovement* movement = bot->GetMovementInterface();
		m_movecaps.Init(movement);
		m_moveiface = movement;
		m_ignoreDanger = false;
		m_teamindex = bot->GetCurrentTeamIndex();
	}

	void SetIgnoreDanger(bool state) { m_ignoreDanger = state; }
	bool IsIngoringDanger() const { return m_ignoreDanger; }
	void SetTeamIndex(int index) { m_teamindex = index; }
	int GetTeamIndex() const { return m_teamindex; }

protected:
	HumanMovementCaps_t m_movecaps;
	IMovement* m_moveiface;
	bool m_ignoreDanger;
	int m_teamindex;

	// Basic ground movement costs
	float GetGroundMovementCost(CNavArea* toArea, CNavArea* fromArea, const CNavLadder* ladder, const NavOffMeshConnection* link, const CNavElevator* elevator, float length) const;
};

#endif // !__NAVBOT_BOT_PATHCOSTS_H_
