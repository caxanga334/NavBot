#include NAVBOT_PCH_FILE
#include <util/pawnutils.h>
#include <sourcepawn/spmanager.h>
#include <navmesh/nav_mesh.h>
#include "navareavector.h"

namespace natives::navmesh::navareavector
{
	static cell_t Constructor(IPluginContext* context, const cell_t* params)
	{
		std::vector<CNavArea*>* vec = nullptr;
		SourceMod::Handle_t handle = createhandle(context, &vec);

		if (handle == BAD_HANDLE)
		{
			return BAD_HANDLE;
		}

		vec->reserve(1024);
		return handle;
	}
	static cell_t Push(IPluginContext* context, const cell_t* params)
	{
		std::vector<CNavArea*>* vec = nullptr;
		SourceMod::Handle_t handle = params[1];
		SourceMod::HandleSecurity security(context->GetIdentity(), myself->GetIdentity());

		if (!pawnutils::ReadHandle("std::vector<CNavArea*>", context, spmanager->GetNavAreaVectorHandleType(), handle, &security, &vec))
		{
			return 0;
		}

		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 2);

		if (!area)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		vec->push_back(area);
		return 0;
	}
	static cell_t Contains(IPluginContext* context, const cell_t* params)
	{
		std::vector<CNavArea*>* vec = nullptr;
		SourceMod::Handle_t handle = params[1];
		SourceMod::HandleSecurity security(context->GetIdentity(), myself->GetIdentity());

		if (!pawnutils::ReadHandle("std::vector<CNavArea*>", context, spmanager->GetNavAreaVectorHandleType(), handle, &security, &vec))
		{
			return 0;
		}

		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 2);

		if (!area)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		for (auto it = vec->begin(); it != vec->end(); it++)
		{
			CNavArea* other = *it;

			if (other == area)
			{
				return 1;
			}
		}

		return 0;
	}
	static cell_t Erase(IPluginContext* context, const cell_t* params)
	{
		std::vector<CNavArea*>* vec = nullptr;
		SourceMod::Handle_t handle = params[1];
		SourceMod::HandleSecurity security(context->GetIdentity(), myself->GetIdentity());

		if (!pawnutils::ReadHandle("std::vector<CNavArea*>", context, spmanager->GetNavAreaVectorHandleType(), handle, &security, &vec))
		{
			return 0;
		}

		CNavArea* area = pawnutils::UnsafeCastPawnAddressToObject<CNavArea>(context, params, 2);

		if (!area)
		{
			context->ReportError("NULL nav area!");
			return 0;
		}

		for (auto it = vec->begin(); it != vec->end(); it++)
		{
			CNavArea* other = *it;

			if (other == area)
			{
				vec->erase(it);
				return 0;
			}
		}

		return 0;
	}
	static cell_t Clear(IPluginContext* context, const cell_t* params)
	{
		std::vector<CNavArea*>* vec = nullptr;
		SourceMod::Handle_t handle = params[1];
		SourceMod::HandleSecurity security(context->GetIdentity(), myself->GetIdentity());

		if (!pawnutils::ReadHandle("std::vector<CNavArea*>", context, spmanager->GetNavAreaVectorHandleType(), handle, &security, &vec))
		{
			return 0;
		}

		vec->clear();
		return 0;
	}
	static cell_t At(IPluginContext* context, const cell_t* params)
	{
		std::size_t firstparam = pawnutils::GetIndexOfParam(context, 1);
		std::vector<CNavArea*>* vec = nullptr;
		SourceMod::Handle_t handle = params[firstparam];
		SourceMod::HandleSecurity security(context->GetIdentity(), myself->GetIdentity());

		if (!pawnutils::ReadHandle("std::vector<CNavArea*>", context, spmanager->GetNavAreaVectorHandleType(), handle, &security, &vec))
		{
			return 0;
		}

		CNavArea* area = nullptr;

		try
		{
			area = vec->at(static_cast<std::size_t>(params[firstparam + 1]));
		}
		catch (const std::out_of_range& ex)
		{
			context->ReportError("Param %i: %s", params[firstparam + 1], ex.what());
			return 0;
		}

		return pawnutils::ReturnPointerToPawn(context, params, area);
	}
	static cell_t Size(IPluginContext* context, const cell_t* params)
	{
		std::vector<CNavArea*>* vec = nullptr;
		SourceMod::Handle_t handle = params[1];
		SourceMod::HandleSecurity security(context->GetIdentity(), myself->GetIdentity());

		if (!pawnutils::ReadHandle("std::vector<CNavArea*>", context, spmanager->GetNavAreaVectorHandleType(), handle, &security, &vec))
		{
			return 0;
		}

		return static_cast<cell_t>(vec->size());
	}

	void setup(std::vector<sp_nativeinfo_t>& nv)
	{
		sp_nativeinfo_t list[] = {
			{"NavBotNavAreaVector.NavBotNavAreaVector", Constructor},
			{"NavBotNavAreaVector.Push", Push},
			{"NavBotNavAreaVector.Contains", Contains},
			{"NavBotNavAreaVector.Erase", Erase},
			{"NavBotNavAreaVector.Clear", Clear},
			{"NavBotNavAreaVector.At", At},
			{"NavBotNavAreaVector.Size.get", Size},
		};

		nv.insert(nv.end(), std::begin(list), std::end(list));
	}

	SourceMod::Handle_t createhandle(SourcePawn::IPluginContext* context, std::vector<CNavArea*>** object)
	{
		std::vector<CNavArea*>* vec = new std::vector<CNavArea*>;
		SourceMod::Handle_t handle = BAD_HANDLE;

		if (!pawnutils::CreateHandle("std::vector<CNavArea*>", spmanager->GetNavAreaVectorHandleType(), vec, context, handle))
		{
			delete vec;
			return BAD_HANDLE;
		}

		*object = vec;
		return handle;
	}
}