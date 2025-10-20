#include NAVBOT_PCH_FILE
#include <extension.h>
#include <manager.h>
#include <util/helpers.h>
#include <bot/basebot.h>
#include <bot/interfaces/path/basepath.h>
#include <bot/interfaces/path/pluginnavigator.h>
#include "path.h"

#define METHODMAP_VALIDNAVIGATOR																			\
	cell_t index = params[1];																				\
																											\
	CPluginMeshNavigator* navigator = extmanager->GetPawnMemoryManager()->GetNavigatorByIndex(index);		\
																											\
	if (navigator == nullptr)																				\
	{																										\
		context->ReportError("Invalid MeshNavigator instance!");											\
		return 0;																							\
	}																										\
																											\

// navigator is first param, bot is second
#define METHODMAP_VALIDNAVANDBOT																			\
	cell_t index = params[1];																				\
																											\
	CPluginMeshNavigator* navigator = extmanager->GetPawnMemoryManager()->GetNavigatorByIndex(index);		\
																											\
	if (navigator == nullptr)																				\
	{																										\
		context->ReportError("Invalid MeshNavigator instance!");											\
		return 0;																							\
	}																										\
																											\
	CBaseBot* bot = extmanager->GetBotByIndex(params[2]);													\
																											\
	if (bot == nullptr || !bot->IsPluginBot())																\
	{																										\
		context->ReportError("%i is not a valid NavBot Plugin Bot instance!", params[2]);					\
		return 0;																							\
	}																										\
																											\

class CPluginBotPathCost : public IPathCost
{
public:
	CPluginBotPathCost(CBaseBot* bot)
	{
		this->m_me = bot;
		IMovement* mover = bot->GetMovementInterface();
		m_stepheight = mover->GetStepHeight();
		m_maxjumpheight = mover->GetMaxJumpHeight();
		m_maxdropheight = mover->GetMaxDropHeight();
		m_maxdjheight = mover->GetMaxDoubleJumpHeight();
		m_maxgapjumpdistance = mover->GetMaxGapJumpDistance();
		m_candoublejump = mover->IsAbleToDoubleJump();
	}

	float operator()(CNavArea* toArea, CNavArea* fromArea, const CNavLadder* ladder, const NavOffMeshConnection* link, const CNavElevator* elevator, float length) const override;

	void SetRouteType([[maybe_unused]] RouteType type) override {}
	RouteType GetRouteType() const override { return DEFAULT_ROUTE; }

private:
	CBaseBot* m_me;
	float m_stepheight;
	float m_maxjumpheight;
	float m_maxdropheight;
	float m_maxdjheight; // max double jump height
	float m_maxgapjumpdistance;
	bool m_candoublejump;
};

float CPluginBotPathCost::operator()(CNavArea* toArea, CNavArea* fromArea, const CNavLadder* ladder, const NavOffMeshConnection* link, const CNavElevator* elevator, float length) const
{
	if (fromArea == nullptr)
	{
		// first area in path, no cost
		return 0.0f;
	}

	if (!m_me->GetMovementInterface()->IsAreaTraversable(toArea))
	{
		return -1.0f;
	}

	float dist = 0.0f;

	if (link != nullptr)
	{
		dist = link->GetConnectionLength();
	}
	else if (elevator != nullptr)
	{
		dist = elevator->GetLengthBetweenFloors(fromArea, toArea);
	}
	else if (length > 0.0f)
	{
		dist = length;
	}
	else
	{
		dist = (toArea->GetCenter() + fromArea->GetCenter()).Length();
	}

	// only check gap and height on common connections
	if (link == nullptr)
	{
		float deltaZ = fromArea->ComputeAdjacentConnectionHeightChange(toArea);

		if (deltaZ >= m_stepheight)
		{
			if (deltaZ >= m_maxjumpheight && !m_candoublejump) // can't double jump
			{
				// too high to reach
				return -1.0f;
			}

			if (deltaZ >= m_maxdjheight) // can double jump
			{
				// too high to reach
				return -1.0f;
			}

			// jump type is resolved by the navigator

			// add jump penalty
			constexpr auto jumpPenalty = 2.0f;
			dist *= jumpPenalty;
		}
		else if (deltaZ < -m_maxdropheight)
		{
			// too far to drop
			return -1.0f;
		}

		float gap = fromArea->ComputeAdjacentConnectionGapDistance(toArea);

		if (gap >= m_maxgapjumpdistance)
		{
			return -1.0f; // can't jump over this gap
		}
	}
	else
	{
		// Don't use double jump links if we can't perform a double jump
		if (link->GetType() == OffMeshConnectionType::OFFMESH_DOUBLE_JUMP && !m_candoublejump)
		{
			return -1.0f;
		}

		// TO-DO: Same check for when rocket jumps are implemented.
	}

	float cost = dist + fromArea->GetCostSoFar();

	return cost;
}

void natives::bots::interfaces::path::setup(std::vector<sp_nativeinfo_t>& nv)
{
	sp_nativeinfo_t list[] = {
		{"MeshNavigator.MeshNavigator", CreateNewPath},
		{"MeshNavigator.Destroy", DestroyNavigator},
		{"MeshNavigator.Update", UpdateNavigator},
		{"MeshNavigator.IsValid", IsValid},
		{"MeshNavigator.ComputeToPos", ComputeToPos},
		{"MeshNavigator.GetMoveGoal", GetMoveGoal},
	};

	nv.insert(nv.end(), std::begin(list), std::end(list));
}

cell_t natives::bots::interfaces::path::CreateNewPath(IPluginContext* context, const cell_t* params)
{
	return extmanager->GetPawnMemoryManager()->AllocNewNavigator();
}

cell_t natives::bots::interfaces::path::DestroyNavigator(IPluginContext* context, const cell_t* params)
{
	cell_t index = params[1];

	if (index <= 0)
	{
		context->ReportError("Invalid MeshNavigator instance!");
		return 0;
	}

	extmanager->GetPawnMemoryManager()->DeleteNavigator(index);
	return 0;
}

cell_t natives::bots::interfaces::path::UpdateNavigator(IPluginContext* context, const cell_t* params)
{
	cell_t index = params[1];

	CPluginMeshNavigator* navigator = extmanager->GetPawnMemoryManager()->GetNavigatorByIndex(index);

	if (navigator == nullptr)
	{
		context->ReportError("Invalid MeshNavigator instance!");
		return 0;
	}

	CBaseBot* bot = extmanager->GetBotByIndex(params[2]);

	if (bot == nullptr || !bot->IsPluginBot())
	{
		context->ReportError("%i is not a valid NavBot Plugin Bot instance!", params[2]);
		return 0;
	}

	navigator->Update(bot);

	return 0;
}

cell_t natives::bots::interfaces::path::IsValid(IPluginContext* context, const cell_t* params)
{
	METHODMAP_VALIDNAVIGATOR;

	return navigator->IsValid() ? 1 : 0;
}

cell_t natives::bots::interfaces::path::ComputeToPos(IPluginContext* context, const cell_t* params)
{
	METHODMAP_VALIDNAVANDBOT;

	cell_t* p3arr = nullptr;

	context->LocalToPhysAddr(params[3], &p3arr);

	Vector goal;
	goal.x = sp_ctof(p3arr[0]);
	goal.y = sp_ctof(p3arr[1]);
	goal.z = sp_ctof(p3arr[2]);

	float maxLength = sp_ctof(params[4]);
	bool includeGoalOnFail = static_cast<bool>(params[5]);
	CPluginBotPathCost pathCost(bot);

	return navigator->ComputePathToPosition(bot, goal, pathCost, maxLength, includeGoalOnFail) ? 1 : 0;
}

cell_t natives::bots::interfaces::path::GetMoveGoal(IPluginContext* context, const cell_t* params)
{
	METHODMAP_VALIDNAVIGATOR;

	cell_t* goal = nullptr;
	context->LocalToPhysAddr(params[2], &goal);
	const Vector& moveGoal = navigator->GetMoveGoal();
	
	goal[0] = sp_ftoc(moveGoal.x);
	goal[1] = sp_ftoc(moveGoal.y);
	goal[2] = sp_ftoc(moveGoal.z);

	return 0;
}
