#include NAVBOT_PCH_FILE
#include <extension.h>
#include <sourcepawn/spmanager.h>
#include <util/pawnutils.h>
#include <bot/basebot.h>
#include <bot/interfaces/path/meshnavigator.h>
#include <bot/bot_pathcosts.h>
#include "path.h"

class CSourcePawnPathCost : public IGroundPathCost
{
public:
	CSourcePawnPathCost(CBaseBot* bot) :
		IGroundPathCost(bot)
	{
		m_routetype = DEFAULT_ROUTE;
	}

	float operator()(CNavArea* toArea, CNavArea* fromArea, const CNavLadder* ladder, const NavOffMeshConnection* link, const CNavElevator* elevator, float length) const override;

	void SetRouteType(RouteType type) override { m_routetype = type; }
	RouteType GetRouteType() const override { return m_routetype; }

private:
	RouteType m_routetype;
};

float CSourcePawnPathCost::operator()(CNavArea* toArea, CNavArea* fromArea, const CNavLadder* ladder, const NavOffMeshConnection* link, const CNavElevator* elevator, float length) const
{
	float cost = GetGroundMovementCost(toArea, fromArea, ladder, link, elevator, length);

	// TO-DO: SourcePawn callback for custom cost
	// Needs nav mesh natives first, otherwise it's kinda useless

	return cost;
}

static cell_t Native_NewNavigator(IPluginContext* context, const cell_t* params)
{
#if defined(KE_ARCH_X64)
	if (context->GetRuntime()->FindPubvarByName("__Virtual_Address__", nullptr) != SP_ERROR_NONE)
	{
		context->ReportError("Virtual address is required to use the navigator on x64!");
		return 0;
	}
#endif

	SourceMod::Handle_t handle = BAD_HANDLE;
	CMeshNavigator* nav = new CMeshNavigator();

	if (!pawnutils::CreateHandle("CMeshNavigator", spmanager->GetMeshNavigatorHandleType(), nav, context, handle))
	{
		delete nav;
		return handle;
	}

	return handle;
}

static cell_t Native_StartRepathTimer(IPluginContext* context, const cell_t* params)
{
	SourceMod::Handle_t handle = params[1];
	SourceMod::HandleSecurity security{ context->GetIdentity(), myself->GetIdentity() };
	CMeshNavigator* nav = nullptr;

	if (!pawnutils::ReadHandle("CMeshNavigator", context, spmanager->GetMeshNavigatorHandleType(), handle, &security, nav))
	{
		return 0;
	}

	float time = sp_ctof(params[2]);
	nav->StartRepathTimer(time);

	return 0;
}

static cell_t Native_NeedsRepath(IPluginContext* context, const cell_t* params)
{
	SourceMod::Handle_t handle = params[1];
	SourceMod::HandleSecurity security{ context->GetIdentity(), myself->GetIdentity() };
	CMeshNavigator* nav = nullptr;

	if (!pawnutils::ReadHandle("CMeshNavigator", context, spmanager->GetMeshNavigatorHandleType(), handle, &security, nav))
	{
		return 0;
	}

	return nav->NeedsRepath() ? 1 : 0;
}

static cell_t Native_Invalidate(IPluginContext* context, const cell_t* params)
{
	SourceMod::Handle_t handle = params[1];
	SourceMod::HandleSecurity security{ context->GetIdentity(), myself->GetIdentity() };
	CMeshNavigator* nav = nullptr;

	if (!pawnutils::ReadHandle("CMeshNavigator", context, spmanager->GetMeshNavigatorHandleType(), handle, &security, nav))
	{
		return 0;
	}

	nav->Invalidate();
	return 0;
}

void natives::bots::interfaces::path::setup(std::vector<sp_nativeinfo_t>& nv)
{
	sp_nativeinfo_t list[] = {
		{"MeshNavigator.MeshNavigator", Native_NewNavigator},
		{"MeshNavigator.Update", UpdateNavigator},
		{"MeshNavigator.IsValid", IsValid},
		{"MeshNavigator.ComputeToPos", ComputeToPos},
		{"MeshNavigator.GetMoveGoal", GetMoveGoal},
		{"MeshNavigator.StartRepathTimer", Native_StartRepathTimer},
		{"MeshNavigator.NeedsRepath", Native_NeedsRepath},
		{"MeshNavigator.Invalidate", Native_Invalidate},
	};

	nv.insert(nv.end(), std::begin(list), std::end(list));
}

cell_t natives::bots::interfaces::path::UpdateNavigator(IPluginContext* context, const cell_t* params)
{
	SourceMod::Handle_t handle = params[1];
	SourceMod::HandleSecurity security{ context->GetIdentity(), myself->GetIdentity() };
	CMeshNavigator* nav = nullptr;
	
	if (!pawnutils::ReadHandle("CMeshNavigator", context, spmanager->GetMeshNavigatorHandleType(), handle, &security, nav))
	{
		return 0;
	}

	CBaseBot* bot = pawnutils::GetBotOfIndex<CBaseBot>(params[2]);

	if (!bot)
	{
		context->ReportError("NULL bot pointer!");
		return 0;
	}

	nav->Update(bot);

	return 0;
}

cell_t natives::bots::interfaces::path::IsValid(IPluginContext* context, const cell_t* params)
{
	SourceMod::Handle_t handle = params[1];
	SourceMod::HandleSecurity security{ context->GetIdentity(), myself->GetIdentity() };
	CMeshNavigator* nav = nullptr;

	if (!pawnutils::ReadHandle("CMeshNavigator", context, spmanager->GetMeshNavigatorHandleType(), handle, &security, nav))
	{
		return 0;
	}

	return nav->IsValid() ? 1 : 0;
}

cell_t natives::bots::interfaces::path::ComputeToPos(IPluginContext* context, const cell_t* params)
{
	SourceMod::Handle_t handle = params[1];
	SourceMod::HandleSecurity security{ context->GetIdentity(), myself->GetIdentity() };
	CMeshNavigator* nav = nullptr;

	if (!pawnutils::ReadHandle("CMeshNavigator", context, spmanager->GetMeshNavigatorHandleType(), handle, &security, nav))
	{
		return 0;
	}

	CBaseBot* bot = pawnutils::GetBotOfIndex<CBaseBot>(params[2]);

	if (!bot)
	{
		context->ReportError("NULL bot pointer!");
		return 0;
	}

	cell_t* p3arr = nullptr;
	context->LocalToPhysAddr(params[3], &p3arr);
	Vector goal = pawnutils::PawnFloatArrayToVector(p3arr);
	float maxLength = sp_ctof(params[4]);
	bool includeGoalOnFail = static_cast<bool>(params[5]);
	CSourcePawnPathCost cost{ bot };

	return nav->ComputePathToPosition(bot, goal, cost, maxLength, includeGoalOnFail) ? 1 : 0;
}

cell_t natives::bots::interfaces::path::GetMoveGoal(IPluginContext* context, const cell_t* params)
{
	SourceMod::Handle_t handle = params[1];
	SourceMod::HandleSecurity security{ context->GetIdentity(), myself->GetIdentity() };
	CMeshNavigator* nav = nullptr;

	if (!pawnutils::ReadHandle("CMeshNavigator", context, spmanager->GetMeshNavigatorHandleType(), handle, &security, nav))
	{
		return 0;
	}

	cell_t* goal = nullptr;
	context->LocalToPhysAddr(params[2], &goal);
	const Vector& moveGoal = nav->GetMoveToPos();
	pawnutils::VectorToPawnFloatArray(goal, moveGoal);

	return 0;
}
