#include NAVBOT_PCH_FILE
#include <util/pawnutils.h>
#include <sourcepawn/spmanager.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_blocker.h>
#include "navblocker.h"

class CSourcePawnNavBlocker final : public CNavBlocker<CNavArea>
{
public:
	CSourcePawnNavBlocker() :
		m_name("CSourcePawnNavBlocker"), m_entity(nullptr)
	{
		m_handledeleted = false;
		m_init = false;
		m_handle = BAD_HANDLE;
		std::fill(std::begin(m_blocked), std::end(m_blocked), false);
		m_initcallback = forwards->CreateForwardEx(nullptr, SourceMod::ExecType::ET_Ignore, 1, nullptr, SourceMod::ParamType::Param_Cell);
		m_onupdatecallback = forwards->CreateForwardEx(nullptr, SourceMod::ExecType::ET_Ignore, 1, nullptr, SourceMod::ParamType::Param_Cell);
		m_onroundrestartcallback = forwards->CreateForwardEx(nullptr, SourceMod::ExecType::ET_Ignore, 1, nullptr, SourceMod::ParamType::Param_Cell);
		m_onrecomputeinternaldatacallback = forwards->CreateForwardEx(nullptr, SourceMod::ExecType::ET_Ignore, 1, nullptr, SourceMod::ParamType::Param_Cell);
	}

	~CSourcePawnNavBlocker() final
	{
		if (m_handle != BAD_HANDLE)
		{
			// this is for when the blocker gets deleted by the nav mesh
			pawnutils::FreeInternalHandle(m_handle);
			m_handle = BAD_HANDLE;
		}

		forwards->ReleaseForward(m_initcallback);
		forwards->ReleaseForward(m_onupdatecallback);
		forwards->ReleaseForward(m_onroundrestartcallback);
		forwards->ReleaseForward(m_onrecomputeinternaldatacallback);
	}

	// Inherited via INavBlocker
	void PostRegister() final
	{
		CNavBlocker<CNavArea>::PostRegister();
	}

	bool IsValid() const final
	{
		if (IsAreaVectorEmpty()) { return false; }

		if (m_handledeleted) { return false; }

		return true;
	}

	void Update() final
	{
		ExecuteCallback(m_onupdatecallback);
	}

	void OnRoundRestart() final
	{
		ExecuteCallback(m_onroundrestartcallback);
	}

	bool IsBlocked(int teamID) const final
	{
		if (teamID < 0 || teamID >= static_cast<int>(m_blocked.size()))
		{
			for (auto& blocked : m_blocked)
			{
				if (blocked)
				{
					return true;
				}
			}

			return false;
		}

		return m_blocked[teamID];
	}

	void OnRecomputeInternalData() final
	{
		ExecuteCallback(m_onrecomputeinternaldatacallback);
	}

	bool RemoveOnRecompute() const final
	{
		return false;
	}

	const char* GetName() const final
	{
		return m_name.c_str();
	}

	void PrintDebugInfo() const final
	{
		META_CONPRINTF("Handle: %X \n", m_handle);
	}

	void OnHandleDeleted() { m_handledeleted = true; SetHandle(BAD_HANDLE); }
	void SetHandle(SourceMod::Handle_t hdl) { m_handle = hdl; }

	void Init()
	{
		m_init = true;
		ExecuteCallback(m_initcallback);
		m_init = false;
	}

	bool IsInitializing() const { return m_init; }

	void SetName(const char* name)
	{
		if (name)
		{
			m_name.assign(name);
		}
	}

	void AddAreasFromVector(std::vector<CNavArea*>* vec)
	{
		for (auto it = vec->begin(); it != vec->end(); it++)
		{
			m_areas.push_back(*it);
		}
	}

	void SetBlockedStatus(int teamID, bool status)
	{
		if (teamID < 0 || teamID >= static_cast<int>(m_blocked.size()))
		{
			std::fill(std::begin(m_blocked), std::end(m_blocked), status);
			return;
		}

		m_blocked[teamID] = status;
	}

	void AddInitCallback(SourcePawn::IPluginFunction* func) { m_initcallback->AddFunction(func); }
	void AddUpdateCallback(SourcePawn::IPluginFunction* func) { m_onupdatecallback->AddFunction(func); }
	void AddOnRoundRestartCallback(SourcePawn::IPluginFunction* func) { m_onroundrestartcallback->AddFunction(func); }
	void AddOnRecomputeInternalDataCallback(SourcePawn::IPluginFunction* func) { m_onrecomputeinternaldatacallback->AddFunction(func); }

	void SetEntity(CBaseEntity* entity) { m_entity.Set(entity); }
	CBaseEntity* GetEntity() const { return m_entity.Get(); }

private:
	std::string m_name;
	CHandle<CBaseEntity> m_entity;
	bool m_handledeleted;
	bool m_init;
	SourceMod::Handle_t m_handle; // my handle value
	std::array<bool, NAV_TEAMS_ARRAY_SIZE> m_blocked;
	SourceMod::IChangeableForward* m_initcallback;
	SourceMod::IChangeableForward* m_onupdatecallback;
	SourceMod::IChangeableForward* m_onroundrestartcallback;
	SourceMod::IChangeableForward* m_onrecomputeinternaldatacallback;

	void ExecuteCallback(SourceMod::IChangeableForward* cb) const
	{
		if (cb->GetFunctionCount() > 0)
		{
			cb->PushCell(m_handle);
			cb->Execute(nullptr);
		}
	}
};

static void FrameAction_InitBlocker(void* data)
{
	CSourcePawnNavBlocker* blocker = reinterpret_cast<CSourcePawnNavBlocker*>(data);
	blocker->Init();
	TheNavMesh->RegisterNavBlocker(blocker);
}

namespace natives::navmesh::navblocker
{
	static cell_t Constructor(IPluginContext* context, const cell_t* params)
	{
		CSourcePawnNavBlocker* blocker = new CSourcePawnNavBlocker;
		SourceMod::Handle_t handle = BAD_HANDLE;

		if (!pawnutils::CreateHandle("CSourcePawnNavBlocker", spmanager->GetNavBlockerHandleType(), blocker, context, handle))
		{
			delete blocker;
			return BAD_HANDLE;
		}

		SourcePawn::IPluginFunction* initCB;
		if (!pawnutils::ReadFunctionByID(context, params, 1, &initCB))
		{
			delete blocker;
			return BAD_HANDLE;
		}

		SourcePawn::IPluginFunction* updateCB;
		if (!pawnutils::ReadFunctionByID(context, params, 2, &updateCB))
		{
			delete blocker;
			return BAD_HANDLE;
		}

		char* name = pawnutils::ReadStringNULL(context, params, 3);
		blocker->SetName(name);
		blocker->SetHandle(handle);
		blocker->AddInitCallback(initCB);
		blocker->AddUpdateCallback(updateCB);
		// I don't know if it's possible to call a forward while inside a native callback.
		smutils->AddFrameAction(FrameAction_InitBlocker, reinterpret_cast<void*>(blocker));
		return handle;
	}
	static cell_t AddAreas(IPluginContext* context, const cell_t* params)
	{
		CSourcePawnNavBlocker* blocker = nullptr;
		SourceMod::Handle_t handle = params[1];
		SourceMod::HandleSecurity sec(context->GetIdentity(), myself->GetIdentity());

		if (!pawnutils::ReadHandle("CSourcePawnNavBlocker", context, spmanager->GetNavBlockerHandleType(), handle, &sec, &blocker))
		{
			return 0;
		}

		if (!blocker->IsInitializing())
		{
			context->ReportError("This native can only be used inside the initialization callback!");
			return 0;
		}

		std::vector<CNavArea*>* vec = nullptr;
		SourceMod::Handle_t vechandle = params[2];

		if (!pawnutils::ReadHandle("std::vector<CNavArea*>", context, spmanager->GetNavAreaVectorHandleType(), vechandle, &sec, &vec))
		{
			return 0;
		}

		blocker->AddAreasFromVector(vec);
		return 0;
	}
	static cell_t UpdateBlockedStatus(IPluginContext* context, const cell_t* params)
	{
		CSourcePawnNavBlocker* blocker = nullptr;
		SourceMod::Handle_t handle = params[1];
		SourceMod::HandleSecurity sec(context->GetIdentity(), myself->GetIdentity());

		if (!pawnutils::ReadHandle("CSourcePawnNavBlocker", context, spmanager->GetNavBlockerHandleType(), handle, &sec, &blocker))
		{
			return 0;
		}

		int teamID = static_cast<int>(params[2]);
		bool status = pawnutils::ReadBool(params, 3);
		blocker->SetBlockedStatus(teamID, status);
		return 0;
	}
	static cell_t SetRoundRestartCallback(IPluginContext* context, const cell_t* params)
	{
		CSourcePawnNavBlocker* blocker = nullptr;
		SourceMod::Handle_t handle = params[1];
		SourceMod::HandleSecurity sec(context->GetIdentity(), myself->GetIdentity());

		if (!pawnutils::ReadHandle("CSourcePawnNavBlocker", context, spmanager->GetNavBlockerHandleType(), handle, &sec, &blocker))
		{
			return 0;
		}

		SourcePawn::IPluginFunction* callback;
		if (!pawnutils::ReadFunctionByID(context, params, 2, &callback))
		{
			return 0;
		}

		blocker->AddOnRoundRestartCallback(callback);
		return 0;
	}
	static cell_t SetRecomputeInternalDataCallback(IPluginContext* context, const cell_t* params)
	{
		CSourcePawnNavBlocker* blocker = nullptr;
		SourceMod::Handle_t handle = params[1];
		SourceMod::HandleSecurity sec(context->GetIdentity(), myself->GetIdentity());

		if (!pawnutils::ReadHandle("CSourcePawnNavBlocker", context, spmanager->GetNavBlockerHandleType(), handle, &sec, &blocker))
		{
			return 0;
		}

		SourcePawn::IPluginFunction* callback;
		if (!pawnutils::ReadFunctionByID(context, params, 2, &callback))
		{
			return 0;
		}

		blocker->AddOnRecomputeInternalDataCallback(callback);
		return 0;
	}
	static cell_t SetEntity(IPluginContext* context, const cell_t* params)
	{
		CSourcePawnNavBlocker* blocker = nullptr;
		SourceMod::Handle_t handle = params[1];
		SourceMod::HandleSecurity sec(context->GetIdentity(), myself->GetIdentity());

		if (!pawnutils::ReadHandle("CSourcePawnNavBlocker", context, spmanager->GetNavBlockerHandleType(), handle, &sec, &blocker))
		{
			return 0;
		}

		CBaseEntity* entity = pawnutils::ReadEntity(context, params, 2);
		blocker->SetEntity(entity);
		return 0;
	}
	static cell_t GetEntity(IPluginContext* context, const cell_t* params)
	{
		CSourcePawnNavBlocker* blocker = nullptr;
		SourceMod::Handle_t handle = params[1];
		SourceMod::HandleSecurity sec(context->GetIdentity(), myself->GetIdentity());

		if (!pawnutils::ReadHandle("CSourcePawnNavBlocker", context, spmanager->GetNavBlockerHandleType(), handle, &sec, &blocker))
		{
			return 0;
		}

		CBaseEntity* entity = blocker->GetEntity();
		return gamehelpers->EntityToBCompatRef(entity);
	}

	void setup(std::vector<sp_nativeinfo_t>& nv)
	{
		sp_nativeinfo_t list[] = {
			{"NavBotNavBlocker.NavBotNavBlocker", Constructor},
			{"NavBotNavBlocker.AddAreas", AddAreas},
			{"NavBotNavBlocker.UpdateBlockedStatus", UpdateBlockedStatus},
			{"NavBotNavBlocker.OnRoundRestart.set", SetRoundRestartCallback},
			{"NavBotNavBlocker.OnRecomputeInternalData.set", SetRecomputeInternalDataCallback},
			{"NavBotNavBlocker.Entity.set", SetEntity},
			{"NavBotNavBlocker.Entity.get", GetEntity},
		};

		nv.insert(nv.end(), std::begin(list), std::end(list));
	}

	void onhandledeleted(void* object)
	{
		reinterpret_cast<CSourcePawnNavBlocker*>(object)->OnHandleDeleted();
	}
}