#ifndef NAVBOT_PLUGIN_NAVMESH_NAVIGATOR_H_
#define NAVBOT_PLUGIN_NAVMESH_NAVIGATOR_H_

#include "meshnavigator.h"

class CPluginMeshNavigator : public CMeshNavigator
{
public:
	CPluginMeshNavigator();
	~CPluginMeshNavigator() override;

	void Update(CBaseBot* bot) override;

	const Vector& GetMoveGoal() const { return m_moveGoal; }

private:
	Vector m_moveGoal;
};

#endif // !NAVBOT_PLUGIN_NAVMESH_NAVIGATOR_H_