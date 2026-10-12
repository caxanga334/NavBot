#include NAVBOT_PCH_FILE
#include <util/pawnutils.h>
#include <sourcepawn/spmanager.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_pathfind.h>
#include "navareavector.h"
#include "navastar.h"

class CSourcePawnNavAStarSearch : public INavAStarSearch<CNavArea>
{
public:
	CSourcePawnNavAStarSearch()
	{
		constexpr std::array gargs = {
			SourceMod::ParamType::Param_Cell,
			SourceMod::ParamType::Param_Cell,
			SourceMod::ParamType::Param_Cell,
			SourceMod::ParamType::Param_Cell,
			SourceMod::ParamType::Param_Cell
		};

		constexpr std::array hargs = {
			SourceMod::ParamType::Param_Cell,
			SourceMod::ParamType::Param_Array
		};


		m_gcostfwd = forwards->CreateForwardEx(nullptr, SourceMod::ExecType::ET_Single, static_cast<int>(gargs.size()), gargs.data());
		m_hcostfwd = forwards->CreateForwardEx(nullptr, SourceMod::ExecType::ET_Single, static_cast<int>(hargs.size()), hargs.data());
	}

	void RunSearch()
	{
		// clear old results
		Clear();

		if (GetStart() == nullptr)
		{
			return;
		}

		SourceMod::IChangeableForward* gfwd = m_gcostfwd;
		SourceMod::IChangeableForward* hfwd = m_hcostfwd;

		auto gcost = [&gfwd](CNavArea* area, CNavArea* fromArea, const CNavLadder* ladder, const NavOffMeshConnection* link, const CNavElevator* elevator) -> float {
			float cost = 0.0f;

#if SMINTERFACE_EXTENSIONAPI_VERSION >= 9
			sp::CallArgs args;
			pawnutils::PushObjectPtr(args, area);
			pawnutils::PushObjectPtr(args, fromArea);
			pawnutils::PushObjectPtr(args, ladder);
			pawnutils::PushObjectPtr(args, link);
			pawnutils::PushObjectPtr(args, elevator);
			cell_t result = 0;
			gfwd->Execute(args, &result);
			cost = sp_ctof(result);

#else // SourceMod 1.12

			pawnutils::PushObjectPtr(gfwd, area);
			pawnutils::PushObjectPtr(gfwd, fromArea);
			pawnutils::PushObjectPtr(gfwd, ladder);
			pawnutils::PushObjectPtr(gfwd, link);
			pawnutils::PushObjectPtr(gfwd, elevator);
			cell_t result = 0;
			gfwd->Execute(&result);
			cost = sp_ctof(result);

#endif // SMINTERFACE_EXTENSIONAPI_VERSION >= 9

			return cost;
		};

		auto hcost = [&hfwd](CNavArea* area, const Vector& goal) -> float {
			float cost = 0.0f;
			cell_t result = 0;
			std::array<cell_t, 3> arr;
			arr[0] = sp_ftoc(goal.x);
			arr[1] = sp_ftoc(goal.y);
			arr[2] = sp_ftoc(goal.z);

#if SMINTERFACE_EXTENSIONAPI_VERSION >= 9
			sp::CallArgs args;
			pawnutils::PushObjectPtr(args, area);
			args.PushArray(arr.data(), static_cast<unsigned int>(arr.size()));
			hfwd->Execute(args, &result);
#else
			pawnutils::PushObjectPtr(hfwd, area);
			hfwd->PushArray(arr.data(), static_cast<unsigned int>(arr.size()));
			hfwd->Execute(&result);
#endif // SMINTERFACE_EXTENSIONAPI_VERSION >= 9

			cost = sp_ctof(result);
			return cost;
		};

		DoSearch(gcost, hcost);
	}

	~CSourcePawnNavAStarSearch() override
	{
		forwards->ReleaseForward(m_gcostfwd);
		m_gcostfwd = nullptr;
		forwards->ReleaseForward(m_hcostfwd);
		m_hcostfwd = nullptr;
	}

	void AddGCostCallback(SourcePawn::IPluginFunction* func)
	{
		m_gcostfwd->AddFunction(func);
	}

	void AddHCostCallback(SourcePawn::IPluginFunction* func)
	{
		m_hcostfwd->AddFunction(func);
	}

	bool AreCallbacksSet() const
	{
		return m_gcostfwd->GetFunctionCount() > 0 && m_hcostfwd->GetFunctionCount() > 0;
	}

private:
	SourceMod::IChangeableForward* m_gcostfwd;
	SourceMod::IChangeableForward* m_hcostfwd;
};


namespace natives::navmesh::navastar
{
	static CSourcePawnNavAStarSearch* readhandle(SourcePawn::IPluginContext* context, SourceMod::Handle_t handle)
	{
		CSourcePawnNavAStarSearch* obj = nullptr;
		SourceMod::HandleSecurity security(context->GetIdentity(), myself->GetIdentity());

		if (!pawnutils::ReadHandle("CSourcePawnNavAStarSearch", context, spmanager->GetNavAStarSearchHandleType(), handle, &security, &obj))
		{
			return nullptr;
		}

		return obj;
	}

	static SourceMod::Handle_t createhandle(SourcePawn::IPluginContext* context, CSourcePawnNavAStarSearch** object)
	{
		CSourcePawnNavAStarSearch* search = new CSourcePawnNavAStarSearch;
		SourceMod::Handle_t handle = BAD_HANDLE;

		if (!pawnutils::CreateHandle("CSourcePawnNavAStarSearch", spmanager->GetNavAStarSearchHandleType(), search, context, handle))
		{
			delete search;
			return BAD_HANDLE;
		}

		*object = search;
		return handle;
	}

	static cell_t Constructor(IPluginContext* context, const cell_t* params)
	{
		CSourcePawnNavAStarSearch* obj = nullptr;
		SourceMod::Handle_t handle = createhandle(context, &obj);

		if (handle == BAD_HANDLE)
		{
			return BAD_HANDLE;
		}

		return handle;
	}
	static cell_t SetGCostCallback(IPluginContext* context, const cell_t* params)
	{
		CSourcePawnNavAStarSearch* obj = readhandle(context, params[1]);

		if (obj == nullptr)
		{
			return 0;
		}

		SourcePawn::IPluginFunction* cb = nullptr;
		if (!pawnutils::ReadFunctionByID(context, params, 2, &cb))
		{
			return 0;
		}

		obj->AddGCostCallback(cb);

		return 0;
	}
	static cell_t SetHCostCallback(IPluginContext* context, const cell_t* params)
	{
		CSourcePawnNavAStarSearch* obj = readhandle(context, params[1]);

		if (obj == nullptr)
		{
			return 0;
		}

		SourcePawn::IPluginFunction* cb = nullptr;
		if (!pawnutils::ReadFunctionByID(context, params, 2, &cb))
		{
			return 0;
		}

		obj->AddHCostCallback(cb);

		return 0;
	}
	static cell_t SetStartArea(IPluginContext* context, const cell_t* params)
	{
		CSourcePawnNavAStarSearch* obj = readhandle(context, params[1]);

		if (obj == nullptr)
		{
			return 0;
		}

		CNavArea* start = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 2);

		if (start == nullptr)
		{
			context->ReportError("NULL start area!");
			return 0;
		}

		obj->SetStart(start);

		return 0;
	}
	static cell_t SetGoalPosition(IPluginContext* context, const cell_t* params)
	{
		CSourcePawnNavAStarSearch* obj = readhandle(context, params[1]);

		if (obj == nullptr)
		{
			return 0;
		}

		Vector goal = pawnutils::ReadVector(context, params, 2);
		obj->SetGoalPosition(goal);

		return 0;
	}
	static cell_t GetTotalCost(IPluginContext* context, const cell_t* params)
	{
		CSourcePawnNavAStarSearch* obj = readhandle(context, params[1]);

		if (obj == nullptr)
		{
			return 0;
		}

		return pawnutils::ReturnFloat(obj->GetTotalCost());
	}
	static cell_t GetFoundPath(IPluginContext* context, const cell_t* params)
	{
		CSourcePawnNavAStarSearch* obj = readhandle(context, params[1]);

		if (obj == nullptr)
		{
			return 0;
		}

		return pawnutils::ReturnBool(obj->FoundPath());
	}
	static cell_t GetNodeForArea(IPluginContext* context, const cell_t* params)
	{
		std::size_t firstparam = pawnutils::GetIndexOfParam(context, 1);

		CSourcePawnNavAStarSearch* obj = readhandle(context, params[firstparam]);

		if (obj == nullptr)
		{
			return 0;
		}

		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, firstparam + 1);

		if (area == nullptr)
		{
			context->ReportError("NULL area!");
			return 0;
		}

		auto node = obj->FindNodeForArea(area);
		return pawnutils::ReturnPointerToPawn(context, params, node);
	}
	static cell_t GetPath(IPluginContext* context, const cell_t* params)
	{
		CSourcePawnNavAStarSearch* obj = readhandle(context, params[1]);

		if (obj == nullptr)
		{
			return 0;
		}

		std::vector<CNavArea*>* vec = nullptr;
		SourceMod::Handle_t handle = natives::navmesh::navareavector::createhandle(context, &vec);

		if (handle == BAD_HANDLE)
		{
			return BAD_HANDLE;
		}

		auto& path = obj->GetPath();

		for (CNavArea* area : path)
		{
			vec->push_back(area);
		}

		return handle;
	}
	static cell_t RunSearch(IPluginContext* context, const cell_t* params)
	{
		CSourcePawnNavAStarSearch* obj = readhandle(context, params[1]);

		if (obj == nullptr)
		{
			return 0;
		}

		if (!obj->AreCallbacksSet())
		{
			context->ReportError("Cost callbacks not set!");
			return 0;
		}

		obj->RunSearch();
		return 0;
	}
	static cell_t Node_GetArea(IPluginContext* context, const cell_t* params)
	{
		std::size_t firstparam = pawnutils::GetIndexOfParam(context, 1);
		NavAStarNode* node = pawnutils::UnsafeCastPawnAddressToObject<NavAStarNode>(context, params, firstparam);

		if (node == nullptr)
		{
			context->ReportError("NULL node!");
			return 0;
		}

		return pawnutils::ReturnPointerToPawn(context, params, node->area);
	}
	static cell_t Node_GetParent(IPluginContext* context, const cell_t* params)
	{
		std::size_t firstparam = pawnutils::GetIndexOfParam(context, 1);
		NavAStarNode* node = pawnutils::UnsafeCastPawnAddressToObject<NavAStarNode>(context, params, firstparam);

		if (node == nullptr)
		{
			context->ReportError("NULL node!");
			return 0;
		}

		return pawnutils::ReturnPointerToPawn(context, params, node->parent);
	}
	static cell_t Node_GetTraverseType(IPluginContext* context, const cell_t* params)
	{
		NavAStarNode* node = pawnutils::UnsafeCastPawnAddressToObject<NavAStarNode>(context, params, 1);

		if (node == nullptr)
		{
			context->ReportError("NULL node!");
			return 0;
		}

		return static_cast<cell_t>(node->how);
	}

	void setup(std::vector<sp_nativeinfo_t>& nv)
	{
		sp_nativeinfo_t list[] = {
			// A Star Search
			{"NavBotNavAStarSearch.NavBotNavAStarSearch", Constructor},
			{"NavBotNavAStarSearch.GCostCallback.set", SetGCostCallback},
			{"NavBotNavAStarSearch.HCostCallback.set", SetHCostCallback},
			{"NavBotNavAStarSearch.StartArea.set", SetStartArea},
			{"NavBotNavAStarSearch.SetGoalPosition", SetGoalPosition},
			{"NavBotNavAStarSearch.TotalCost.get", GetTotalCost},
			{"NavBotNavAStarSearch.FoundPath.get", GetFoundPath},
			{"NavBotNavAStarSearch.GetNodeForArea", GetNodeForArea},
			{"NavBotNavAStarSearch.GetPath", GetPath},
			{"NavBotNavAStarSearch.RunSearch", RunSearch},

			// Search Node
			{"NavBotNavAStarSearchNode.GetArea", Node_GetArea},
			{"NavBotNavAStarSearchNode.GetParent", Node_GetParent},
			{"NavBotNavAStarSearchNode.GetTraverseType", Node_GetTraverseType},
		};

		nv.insert(nv.end(), std::begin(list), std::end(list));
	}

	void destroy(void* object)
	{
		delete reinterpret_cast<CSourcePawnNavAStarSearch*>(object);
	}
}