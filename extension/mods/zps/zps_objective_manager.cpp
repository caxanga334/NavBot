#include NAVBOT_PCH_FILE
#include "nav/zps_nav_mesh.h"
#include "zps_mod.h"

CZPSObjectiveManager::CZPSObjectiveManager() :
	m_moveTo(0.0f, 0.0f, 0.0f)
{
	m_currentObjective = ObjectiveTypes::OBJECTIVE_NONE;
	m_itemID.reserve(32);
	m_detectionRadius = 512.0f;
}

CZPSObjectiveManager::~CZPSObjectiveManager()
{
}

void CZPSObjectiveManager::Reset()
{
	m_currentObjective = ObjectiveTypes::OBJECTIVE_NONE;
	m_moveTo.Init(0.0f, 0.0f, 0.0f);
	m_useButton.Term();
	m_itemID.clear();
	m_itemUseTarget.Term();
	m_detectionRadius = 512.0f;
}
