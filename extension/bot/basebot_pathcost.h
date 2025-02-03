#ifndef NAVBOT_BASE_BOT_PATH_COST_H_
#define NAVBOT_BASE_BOT_PATH_COST_H_

#include "basebot.h"
#include "interfaces/path/basepath.h"

class CBaseBotPathCost : public IPathCost
{
public:
	CBaseBotPathCost(CBaseBot* bot);

	float operator()(CNavArea* toArea, CNavArea* fromArea, const CNavLadder* ladder, const NavOffMeshConnection* link, const CNavElevator* elevator, float length) const override;

private:
	CBaseBot* m_me;
	float m_stepheight;
	float m_maxjumpheight;
	float m_maxdropheight;
	float m_maxgapjumpdistance;
	float m_maxdjheight;
	bool m_candoublejump;
	bool m_canblastjump;
};

#endif // !NAVBOT_BASE_BOT_PATH_COST_H_
