#include NAVBOT_PCH_FILE
#include <util/pawnutils.h>
#include <sourcepawn/spmanager.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_pathfind.h>
#include "navareavector.h"
#include "navcollector.h"

class CSourcePawnAreaCollector final : public INavAreaCollector<CNavArea>
{
public:
	CSourcePawnAreaCollector() :
		INavAreaCollector<CNavArea>()
	{
		m_shouldsearchcallback = forwards->CreateForwardEx(nullptr, SourceMod::ExecType::ET_Single, 1, nullptr, SourceMod::ParamType::Param_Cell);
		m_shouldcollectcallback = forwards->CreateForwardEx(nullptr, SourceMod::ExecType::ET_Single, 1, nullptr, SourceMod::ParamType::Param_Cell);
	}

	bool ShouldSearch(CNavArea* area) final
	{
		if (m_shouldsearchcallback->GetFunctionCount() > 0)
		{
			cell_t res = 1;
#if SM_BUILD_MINOR_INT >= SM_VERSION_1_13			
			sp::CallArgs args;
			pawnutils::PushObjectPtr(args, area);		
			m_shouldsearchcallback->Execute(args, &res);
#else
			pawnutils::PushObjectPtr(m_shouldsearchcallback, area);
			m_shouldsearchcallback->Execute(&res);
#endif // SM_BUILD_MINOR_INT >= SM_VERSION_1_13
			return res != 0;
		}

		return true;
	}

	bool ShouldCollect(CNavArea* area) final
	{
		if (m_shouldcollectcallback->GetFunctionCount() > 0)
		{
			cell_t res = 1;
#if SM_BUILD_MINOR_INT >= SM_VERSION_1_13			
			sp::CallArgs args;
			pawnutils::PushObjectPtr(args, area);
			m_shouldcollectcallback->Execute(args, &res);
#else
			pawnutils::PushObjectPtr(m_shouldcollectcallback, area);
			m_shouldcollectcallback->Execute(&res);
#endif // SM_BUILD_MINOR_INT >= SM_VERSION_1_13
			return res != 0;
		}

		return true;
	}

	float ComputeCostBetweenAreas(CNavArea* prevArea, CNavArea* nextArea) final
	{
		// TO-DO: Add a callback for this.
		// Need to expose more natives of CNavArea before plugins can really use this.
		return INavAreaCollector<CNavArea>::ComputeCostBetweenAreas(prevArea, nextArea);
	}

	void AddShouldSearchCallback(SourcePawn::IPluginFunction* function) { m_shouldsearchcallback->AddFunction(function); }
	void AddShouldCollectCallback(SourcePawn::IPluginFunction* function) { m_shouldcollectcallback->AddFunction(function); }
private:
	SourceMod::IChangeableForward* m_shouldsearchcallback;
	SourceMod::IChangeableForward* m_shouldcollectcallback;
};

namespace natives::navmesh::searchnodes
{
	static cell_t GetMe(IPluginContext* context, const cell_t* params)
	{
		const INavSearchNode<CNavArea>* node = pawnutils::UnsafeCastPawnAddressToObject<const INavSearchNode<CNavArea>>(context, params, pawnutils::GetIndexOfParam(context, 1));

		if (!node)
		{
			context->ReportError("NULL node!");
			return 0;
		}


		return pawnutils::ReturnPointerToPawn(context, params, node->GetArea());
	}
	static cell_t GetParent(IPluginContext* context, const cell_t* params)
	{
		const INavSearchNode<CNavArea>* node = pawnutils::UnsafeCastPawnAddressToObject<const INavSearchNode<CNavArea>>(context, params, pawnutils::GetIndexOfParam(context, 1));

		if (!node)
		{
			context->ReportError("NULL node!");
			return 0;
		}


		return pawnutils::ReturnPointerToPawn(context, params, node->parent);
	}
	static cell_t GetTravelCostFromParent(IPluginContext* context, const cell_t* params)
	{
		const INavSearchNode<CNavArea>* node = pawnutils::UnsafeCastPawnAddressToObject<const INavSearchNode<CNavArea>>(context, params, 1);

		if (!node)
		{
			context->ReportError("NULL node!");
			return 0;
		}


		return pawnutils::ReturnFloat(node->GetTravelCostFromParent());
	}
	static cell_t GetTravelCostFromStart(IPluginContext* context, const cell_t* params)
	{
		const INavSearchNode<CNavArea>* node = pawnutils::UnsafeCastPawnAddressToObject<const INavSearchNode<CNavArea>>(context, params, 1);

		if (!node)
		{
			context->ReportError("NULL node!");
			return 0;
		}


		return pawnutils::ReturnFloat(node->GetTravelCostFromStart());
	}
}

namespace natives::navmesh::collector
{
	static cell_t Constructor(IPluginContext* context, const cell_t* params)
	{
#if defined(KE_ARCH_X64)
		if (!pawnutils::IsInt64Available(context))
		{
			context->ReportError("This native requires int64 (SM 1.13+) on 64 bits!");
			return BAD_HANDLE;
		}
#endif // defined(KE_ARCH_X64)

		CSourcePawnAreaCollector* collector = new CSourcePawnAreaCollector();
		SourceMod::Handle_t handle = BAD_HANDLE;
		
		if (!pawnutils::CreateHandle("CSourcePawnAreaCollector", spmanager->GetNavCollectorHandleType(), collector, context, handle))
		{
			delete collector;
			return BAD_HANDLE;
		}

		return handle;
	}
	static cell_t SetSearchStartArea(IPluginContext* context, const cell_t* params)
	{
		SourceMod::Handle_t handle = params[1];
		CSourcePawnAreaCollector* collector = nullptr;
		SourceMod::HandleSecurity security(context->GetIdentity(), myself->GetIdentity());

		if (!pawnutils::ReadHandle("CSourcePawnAreaCollector", context, spmanager->GetNavCollectorHandleType(), handle, &security, &collector))
		{
			return 0;
		}

		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 2);

		if (!area)
		{
			context->ReportError("Start area cannot be NULL!");
			return 0;
		}

		collector->SetStartArea(area);
		return 0;
	}
	static cell_t SetShouldSearchCallback(IPluginContext* context, const cell_t* params)
	{
		SourceMod::Handle_t handle = params[1];
		CSourcePawnAreaCollector* collector = nullptr;
		SourceMod::HandleSecurity security(context->GetIdentity(), myself->GetIdentity());

		if (!pawnutils::ReadHandle("CSourcePawnAreaCollector", context, spmanager->GetNavCollectorHandleType(), handle, &security, &collector))
		{
			return 0;
		}

		SourcePawn::IPluginFunction* function = nullptr;
		
		if (!pawnutils::ReadFunctionByID(context, params, 2, &function))
		{
			return 0;
		}

		collector->AddShouldSearchCallback(function);
		return 0;
	}
	static cell_t SetShouldCollectCallback(IPluginContext* context, const cell_t* params)
	{
		SourceMod::Handle_t handle = params[1];
		CSourcePawnAreaCollector* collector = nullptr;
		SourceMod::HandleSecurity security(context->GetIdentity(), myself->GetIdentity());

		if (!pawnutils::ReadHandle("CSourcePawnAreaCollector", context, spmanager->GetNavCollectorHandleType(), handle, &security, &collector))
		{
			return 0;
		}

		SourcePawn::IPluginFunction* function = nullptr;

		if (!pawnutils::ReadFunctionByID(context, params, 2, &function))
		{
			return 0;
		}

		collector->AddShouldCollectCallback(function);
		return 0;
	}
	static cell_t SetTravelLimit(IPluginContext* context, const cell_t* params)
	{
		SourceMod::Handle_t handle = params[1];
		CSourcePawnAreaCollector* collector = nullptr;
		SourceMod::HandleSecurity security(context->GetIdentity(), myself->GetIdentity());

		if (!pawnutils::ReadHandle("CSourcePawnAreaCollector", context, spmanager->GetNavCollectorHandleType(), handle, &security, &collector))
		{
			return 0;
		}
		
		collector->SetTravelLimit(pawnutils::ReadFloat(params, 2));
		return 0;
	}
	static cell_t GetTravelLimit(IPluginContext* context, const cell_t* params)
	{
		SourceMod::Handle_t handle = params[1];
		CSourcePawnAreaCollector* collector = nullptr;
		SourceMod::HandleSecurity security(context->GetIdentity(), myself->GetIdentity());

		if (!pawnutils::ReadHandle("CSourcePawnAreaCollector", context, spmanager->GetNavCollectorHandleType(), handle, &security, &collector))
		{
			return 0;
		}

		return pawnutils::ReturnFloat(collector->GetTravelLimit());
	}
	static cell_t SetSearchLadder(IPluginContext* context, const cell_t* params)
	{
		SourceMod::Handle_t handle = params[1];
		CSourcePawnAreaCollector* collector = nullptr;
		SourceMod::HandleSecurity security(context->GetIdentity(), myself->GetIdentity());

		if (!pawnutils::ReadHandle("CSourcePawnAreaCollector", context, spmanager->GetNavCollectorHandleType(), handle, &security, &collector))
		{
			return 0;
		}

		collector->SetSearchLadders(pawnutils::ReadBool(params, 2));
		return 0;
	}
	static cell_t SetSearchLinks(IPluginContext* context, const cell_t* params)
	{
		SourceMod::Handle_t handle = params[1];
		CSourcePawnAreaCollector* collector = nullptr;
		SourceMod::HandleSecurity security(context->GetIdentity(), myself->GetIdentity());

		if (!pawnutils::ReadHandle("CSourcePawnAreaCollector", context, spmanager->GetNavCollectorHandleType(), handle, &security, &collector))
		{
			return 0;
		}

		collector->SetSearchLinks(pawnutils::ReadBool(params, 2));
		return 0;
	}
	static cell_t SetSearchElevators(IPluginContext* context, const cell_t* params)
	{
		SourceMod::Handle_t handle = params[1];
		CSourcePawnAreaCollector* collector = nullptr;
		SourceMod::HandleSecurity security(context->GetIdentity(), myself->GetIdentity());

		if (!pawnutils::ReadHandle("CSourcePawnAreaCollector", context, spmanager->GetNavCollectorHandleType(), handle, &security, &collector))
		{
			return 0;
		}

		collector->SetSearchElevators(pawnutils::ReadBool(params, 2));
		return 0;
	}
	static cell_t SetSearchIncomingAreas(IPluginContext* context, const cell_t* params)
	{
		SourceMod::Handle_t handle = params[1];
		CSourcePawnAreaCollector* collector = nullptr;
		SourceMod::HandleSecurity security(context->GetIdentity(), myself->GetIdentity());

		if (!pawnutils::ReadHandle("CSourcePawnAreaCollector", context, spmanager->GetNavCollectorHandleType(), handle, &security, &collector))
		{
			return 0;
		}

		collector->SetSearchIncomingConnections(pawnutils::ReadBool(params, 2));
		return 0;
	}
	static cell_t ExecuteSearch(IPluginContext* context, const cell_t* params)
	{
		SourceMod::Handle_t handle = params[1];
		CSourcePawnAreaCollector* collector = nullptr;
		SourceMod::HandleSecurity security(context->GetIdentity(), myself->GetIdentity());

		if (!pawnutils::ReadHandle("CSourcePawnAreaCollector", context, spmanager->GetNavCollectorHandleType(), handle, &security, &collector))
		{
			return 0;
		}

		collector->Execute();
		return 0;
	}
	static cell_t ResetSearch(IPluginContext* context, const cell_t* params)
	{
		SourceMod::Handle_t handle = params[1];
		CSourcePawnAreaCollector* collector = nullptr;
		SourceMod::HandleSecurity security(context->GetIdentity(), myself->GetIdentity());

		if (!pawnutils::ReadHandle("CSourcePawnAreaCollector", context, spmanager->GetNavCollectorHandleType(), handle, &security, &collector))
		{
			return 0;
		}

		collector->Reset();
		return 0;
	}
	static cell_t EndSearch(IPluginContext* context, const cell_t* params)
	{
		SourceMod::Handle_t handle = params[1];
		CSourcePawnAreaCollector* collector = nullptr;
		SourceMod::HandleSecurity security(context->GetIdentity(), myself->GetIdentity());

		if (!pawnutils::ReadHandle("CSourcePawnAreaCollector", context, spmanager->GetNavCollectorHandleType(), handle, &security, &collector))
		{
			return 0;
		}

		collector->EndSearch();
		return 0;
	}
	static cell_t GetSearchNodeForArea(IPluginContext* context, const cell_t* params)
	{
		std::size_t firstparam = pawnutils::GetIndexOfParam(context, 1);
		SourceMod::Handle_t handle = params[firstparam];
		CSourcePawnAreaCollector* collector = nullptr;
		SourceMod::HandleSecurity security(context->GetIdentity(), myself->GetIdentity());

		if (!pawnutils::ReadHandle("CSourcePawnAreaCollector", context, spmanager->GetNavCollectorHandleType(), handle, &security, &collector))
		{
			return 0;
		}

		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, firstparam + 1);

		if (!area)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		auto node = collector->GetNodeForArea(area);
		return pawnutils::ReturnPointerToPawn(context, params, node);
	}
	static cell_t GetCollectedAreasSize(IPluginContext* context, const cell_t* params)
	{
		SourceMod::Handle_t handle = params[1];
		CSourcePawnAreaCollector* collector = nullptr;
		SourceMod::HandleSecurity security(context->GetIdentity(), myself->GetIdentity());

		if (!pawnutils::ReadHandle("CSourcePawnAreaCollector", context, spmanager->GetNavCollectorHandleType(), handle, &security, &collector))
		{
			return 0;
		}

		return static_cast<cell_t>(collector->GetCollectedAreasCount());
	}
	static cell_t GetRandomCollectedArea(IPluginContext* context, const cell_t* params)
	{
		SourceMod::Handle_t handle = params[pawnutils::GetIndexOfParam(context, 1)];
		CSourcePawnAreaCollector* collector = nullptr;
		SourceMod::HandleSecurity security(context->GetIdentity(), myself->GetIdentity());

		if (!pawnutils::ReadHandle("CSourcePawnAreaCollector", context, spmanager->GetNavCollectorHandleType(), handle, &security, &collector))
		{
			return 0;
		}

		if (collector->IsCollectedAreasEmpty())
		{
			return pawnutils::ReturnPointerToPawn<CNavArea*>(context, params, nullptr);
		}
		
		CNavArea* area = collector->GetRandomCollectedArea();
		return pawnutils::ReturnPointerToPawn(context, params, area);
	}
	static cell_t GetFarthestCollectedArea(IPluginContext* context, const cell_t* params)
	{
		SourceMod::Handle_t handle = params[pawnutils::GetIndexOfParam(context, 1)];
		CSourcePawnAreaCollector* collector = nullptr;
		SourceMod::HandleSecurity security(context->GetIdentity(), myself->GetIdentity());

		if (!pawnutils::ReadHandle("CSourcePawnAreaCollector", context, spmanager->GetNavCollectorHandleType(), handle, &security, &collector))
		{
			return 0;
		}

		CNavArea* area = collector->GetFarthestCollectedArea();
		return pawnutils::ReturnPointerToPawn(context, params, area);
	}
	static cell_t GetNearestCollectedArea(IPluginContext* context, const cell_t* params)
	{
		SourceMod::Handle_t handle = params[pawnutils::GetIndexOfParam(context, 1)];
		CSourcePawnAreaCollector* collector = nullptr;
		SourceMod::HandleSecurity security(context->GetIdentity(), myself->GetIdentity());

		if (!pawnutils::ReadHandle("CSourcePawnAreaCollector", context, spmanager->GetNavCollectorHandleType(), handle, &security, &collector))
		{
			return 0;
		}

		CNavArea* area = collector->GetNearestCollectedArea();
		return pawnutils::ReturnPointerToPawn(context, params, area);
	}
	static cell_t GetCollectedAreas(IPluginContext* context, const cell_t* params)
	{
		SourceMod::Handle_t handle = params[1];
		CSourcePawnAreaCollector* collector = nullptr;
		SourceMod::HandleSecurity security(context->GetIdentity(), myself->GetIdentity());

		if (!pawnutils::ReadHandle("CSourcePawnAreaCollector", context, spmanager->GetNavCollectorHandleType(), handle, &security, &collector))
		{
			return BAD_HANDLE;
		}

		std::vector<CNavArea*>* vec = nullptr;
		SourceMod::Handle_t vechandle = natives::navmesh::navareavector::createhandle(context, &vec);

		if (vechandle == BAD_HANDLE)
		{
			return BAD_HANDLE;
		}

		auto& collected = collector->GetCollectedAreas();

		// copy collected areas in the vector
		for (CNavArea* area : collected)
		{
			vec->push_back(area);
		}

		return vechandle;
	}

	void setup(std::vector<sp_nativeinfo_t>& nv)
	{
		sp_nativeinfo_t list[] = {
			{"NavBotNavAreaCollector.NavBotNavAreaCollector", Constructor},
			{"NavBotNavAreaCollector.SetSearchStartArea", SetSearchStartArea},
			{"NavBotNavAreaCollector.ShouldSearch.set", SetShouldSearchCallback},
			{"NavBotNavAreaCollector.ShouldCollect.set", SetShouldCollectCallback},
			{"NavBotNavAreaCollector.TravelLimit.set", SetTravelLimit},
			{"NavBotNavAreaCollector.TravelLimit.get", GetTravelLimit},
			{"NavBotNavAreaCollector.SearchLadder.set", SetSearchLadder},
			{"NavBotNavAreaCollector.SearchOffMeshLinks.set", SetSearchLinks},
			{"NavBotNavAreaCollector.SearchElevators.set", SetSearchElevators},
			{"NavBotNavAreaCollector.SearchIncoming.set", SetSearchIncomingAreas},
			{"NavBotNavAreaCollector.Execute", ExecuteSearch},
			{"NavBotNavAreaCollector.Reset", ResetSearch},
			{"NavBotNavAreaCollector.EndSearch", EndSearch},
			{"NavBotNavAreaCollector.CollectedAreaSize.get", GetCollectedAreasSize},
			{"NavBotNavAreaCollector.GetRandomCollectedArea", GetRandomCollectedArea},
			{"NavBotNavAreaCollector.GetFarthestCollectedArea", GetFarthestCollectedArea},
			{"NavBotNavAreaCollector.GetNearestCollectedArea", GetNearestCollectedArea},
			{"NavBotNavAreaCollector.GetCollectedAreas", GetCollectedAreas},

			/* INavSearchNode */
			{"NavBotNavAreaSearchNode.GetArea", natives::navmesh::searchnodes::GetMe},
			{"NavBotNavAreaSearchNode.GetParent", natives::navmesh::searchnodes::GetParent},
			{"NavBotNavAreaSearchNode.GetTravelCostFromParent", natives::navmesh::searchnodes::GetTravelCostFromParent},
			{"NavBotNavAreaSearchNode.GetTravelCostFromStart", natives::navmesh::searchnodes::GetTravelCostFromStart},
		};

		nv.insert(nv.end(), std::begin(list), std::end(list));
	}

	void destroy(void* object)
	{
		CSourcePawnAreaCollector* collector = reinterpret_cast<CSourcePawnAreaCollector*>(object);
		delete collector;
	}
}