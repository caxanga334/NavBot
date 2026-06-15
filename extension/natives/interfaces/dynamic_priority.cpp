#include NAVBOT_PCH_FILE
#include <any>
#include <sourcepawn/spmanager.h>
#include <util/pawnutils.h>
#include <bot/basebot.h>
#include <bot/interfaces/weapons/dynamic_priority_manager.h>
#include "dynamic_priority.h"

class CSourcePawnDynamicPriority final : public CBaseDynamicPriority
{
public:
	static constexpr std::size_t MAX_SP_ARGS = 8U;
	static constexpr std::array CB_CONFIGURE_PARAMS = { SourceMod::ParamType::Param_Cell, SourceMod::ParamType::Param_String, SourceMod::ParamType::Param_Cell };
	static constexpr std::array CB_GETPRIO_PARAMS = { 
		SourceMod::ParamType::Param_Cell, 
		SourceMod::ParamType::Param_Cell, 
		SourceMod::ParamType::Param_Cell, 
		SourceMod::ParamType::Param_Cell 
	};

	CSourcePawnDynamicPriority()
	{
		callback_configure = forwards->CreateForwardEx(nullptr, SourceMod::ExecType::ET_Single, static_cast<int>(CB_CONFIGURE_PARAMS.size()), CB_CONFIGURE_PARAMS.data());
		callback_getpriority = forwards->CreateForwardEx(nullptr, SourceMod::ExecType::ET_Single, static_cast<int>(CB_GETPRIO_PARAMS.size()), CB_GETPRIO_PARAMS.data());
	}

	~CSourcePawnDynamicPriority() final
	{
		forwards->ReleaseForward(callback_configure);
	}

	bool Configure(const std::vector<std::string>& args) final
	{
		if (!CBaseDynamicPriority::Configure(args)) { return false; }

		SourceMod::Handle_t handle = BAD_HANDLE;
		CSourcePawnDynamicPriority* object = const_cast<CSourcePawnDynamicPriority*>(this);
		
		if (!pawnutils::CreateInternalHandle("IDynamicWeaponPriority", spmanager->GetWeaponPriorityInstanceHandleType(), object, handle))
		{
			smutils->LogError(myself, "Failed to IDynamicWeaponPriority handle!");
			return true;
		}

		if (callback_configure->GetFunctionCount() > 0)
		{
			for (std::size_t i = 1; i < args.size(); i++)
			{
				callback_configure->PushCell(static_cast<cell_t>(handle));
				callback_configure->PushString(args[i].c_str());
				callback_configure->PushCell(static_cast<cell_t>(i));
				cell_t result = 1;
				callback_configure->Execute(&result);

				if (result == 0)
				{
					pawnutils::FreeInternalHandle(handle);
					return false;
				}
			}
		}

		pawnutils::FreeInternalHandle(handle);
		return true;
	}

	int GetPriorityValue(const CBaseBot* bot, const CBotWeapon* weapon, const CKnownEntity* threat) const final
	{
		cell_t result = 0;

		if (callback_getpriority->GetFunctionCount() > 0)
		{
			SourceMod::Handle_t handle = BAD_HANDLE;
			CSourcePawnDynamicPriority* object = const_cast<CSourcePawnDynamicPriority*>(this);

			if (!pawnutils::CreateInternalHandle("IDynamicWeaponPriority", spmanager->GetWeaponPriorityInstanceHandleType(), object, handle))
			{
				smutils->LogError(myself, "Failed to IDynamicWeaponPriority handle!");
				return 0;
			}

			callback_getpriority->PushCell(static_cast<cell_t>(handle));
			callback_getpriority->PushCell(static_cast<cell_t>(bot->GetIndex()));
			callback_getpriority->PushCell(static_cast<cell_t>(weapon->GetIndex()));
			callback_getpriority->PushCell(static_cast<cell_t>(threat->GetIndex()));
			callback_getpriority->Execute(&result);

			pawnutils::FreeInternalHandle(handle);
		}

		return static_cast<int>(result);
	}

	void AddConfigureCallback(SourcePawn::IPluginFunction* func) { callback_configure->AddFunction(func); }
	void AddGetPriorityCallback(SourcePawn::IPluginFunction* func) { callback_getpriority->AddFunction(func); }

	template <typename T>
	void StoreData(T data, std::size_t index) { args[index] = data; }
	template <typename T>
	T ReadData(std::size_t index) { return std::any_cast<T>(args[index]); }

	int GetBasePriority() const { return GetPriority(); }

private:
	std::array<std::any, MAX_SP_ARGS> args;
	SourceMod::IChangeableForward* callback_configure;
	SourceMod::IChangeableForward* callback_getpriority;
};

class CSourcePawnDynamicFactory : public CDynamicPriorityManager::Factory
{
public:
	CSourcePawnDynamicFactory()
	{
		configure_func = nullptr;
		getpriority_func = nullptr;
	}

	// Inherited via Factory
	IDynamicWeaponPriority* Create() const override
	{
		CSourcePawnDynamicPriority* obj = new CSourcePawnDynamicPriority;

		obj->AddConfigureCallback(configure_func);
		obj->AddGetPriorityCallback(getpriority_func);

		return obj;
	}

	SourcePawn::IPluginFunction* configure_func;
	SourcePawn::IPluginFunction* getpriority_func;
};

namespace natives::bots::interfaces::dynamicpriority
{
	static cell_t Native_CreateDynamicPriotityHandler(IPluginContext* context, const cell_t* params)
	{
		char* name;
		context->LocalToString(params[1], &name);

		CDynamicPriorityManager& manager = CDynamicPriorityManager::GetManager();

		if (manager.FindFactory(name) != nullptr)
		{
			context->ReportError("A dynamic priority factory for \"%s\" already exists!", name);
			return 0;
		}

		SourcePawn::IPluginFunction* configure = context->GetFunctionById(static_cast<funcid_t>(params[2]));
		SourcePawn::IPluginFunction* getprio = context->GetFunctionById(static_cast<funcid_t>(params[3]));

		if (!configure || !getprio)
		{
			context->ReportError("Failed to get callback functions!");
			return BAD_HANDLE;
		}

		CSourcePawnDynamicFactory* object = new CSourcePawnDynamicFactory;
		SourceMod::HandleType_t handle;

		if (!pawnutils::CreateHandle("CSourcePawnDynamicFactory", spmanager->GetWeaponPriorityFactoryHandleType(), object, context, handle))
		{
			delete object;
			return BAD_HANDLE;
		}

		object->configure_func = configure;
		object->getpriority_func = getprio;
		manager.RegisterFactory(name, object);
		return static_cast<cell_t>(handle);
	}
	static cell_t Native_InstanceGetPriority(IPluginContext* context, const cell_t* params)
	{
		SourceMod::Handle_t handle = params[1];
		SourceMod::HandleSecurity security{ context->GetIdentity(), myself->GetIdentity() };
		CSourcePawnDynamicPriority* object = nullptr;

		if (!pawnutils::ReadHandle("IDynamicWeaponPriority", context, spmanager->GetWeaponPriorityInstanceHandleType(), handle, &security, &object))
		{
			return 0;
		}

		return static_cast<cell_t>(object->GetBasePriority());
	}
	static cell_t Native_InstanceStoreInt(IPluginContext* context, const cell_t* params)
	{
		SourceMod::Handle_t handle = params[1];
		SourceMod::HandleSecurity security{ context->GetIdentity(), myself->GetIdentity() };
		CSourcePawnDynamicPriority* object = nullptr;

		if (!pawnutils::ReadHandle("IDynamicWeaponPriority", context, spmanager->GetWeaponPriorityInstanceHandleType(), handle, &security, &object))
		{
			return 0;
		}

		std::size_t index = static_cast<std::size_t>(params[3]);

		if (index >= CSourcePawnDynamicPriority::MAX_SP_ARGS)
		{
			context->ReportError("Index is out of bounds! %i >= %zu", params[3], index);
			return 0;
		}

		object->StoreData(static_cast<int>(params[2]), index);
		return 0;
	}
	static cell_t Native_InstanceStoreFloat(IPluginContext* context, const cell_t* params)
	{
		SourceMod::Handle_t handle = params[1];
		SourceMod::HandleSecurity security{ context->GetIdentity(), myself->GetIdentity() };
		CSourcePawnDynamicPriority* object = nullptr;

		if (!pawnutils::ReadHandle("IDynamicWeaponPriority", context, spmanager->GetWeaponPriorityInstanceHandleType(), handle, &security, &object))
		{
			return 0;
		}

		std::size_t index = static_cast<std::size_t>(params[3]);

		if (index >= CSourcePawnDynamicPriority::MAX_SP_ARGS)
		{
			context->ReportError("Index is out of bounds! %i >= %zu", params[3], index);
			return 0;
		}

		object->StoreData(pawnutils::ReadFloat(params, 2), index);
		return 0;
	}
	static cell_t Native_InstanceStoreString(IPluginContext* context, const cell_t* params)
	{
		SourceMod::Handle_t handle = params[1];
		SourceMod::HandleSecurity security{ context->GetIdentity(), myself->GetIdentity() };
		CSourcePawnDynamicPriority* object = nullptr;

		if (!pawnutils::ReadHandle("IDynamicWeaponPriority", context, spmanager->GetWeaponPriorityInstanceHandleType(), handle, &security, &object))
		{
			return 0;
		}

		std::size_t index = static_cast<std::size_t>(params[3]);

		if (index >= CSourcePawnDynamicPriority::MAX_SP_ARGS)
		{
			context->ReportError("Index is out of bounds! %i >= %zu", params[3], index);
			return 0;
		}

		char* str;
		context->LocalToString(params[2], &str);
		object->StoreData(std::string(str), index);
		return 0;
	}
	static cell_t Native_InstanceReadInt(IPluginContext* context, const cell_t* params)
	{
		SourceMod::Handle_t handle = params[1];
		SourceMod::HandleSecurity security{ context->GetIdentity(), myself->GetIdentity() };
		CSourcePawnDynamicPriority* object = nullptr;

		if (!pawnutils::ReadHandle("IDynamicWeaponPriority", context, spmanager->GetWeaponPriorityInstanceHandleType(), handle, &security, &object))
		{
			return 0;
		}

		std::size_t index = static_cast<std::size_t>(params[2]);

		if (index >= CSourcePawnDynamicPriority::MAX_SP_ARGS)
		{
			context->ReportError("Index is out of bounds! %i >= %zu", params[2], index);
			return 0;
		}

		try
		{
			int value = object->ReadData<int>(index);
			return static_cast<cell_t>(value);
		}
		catch (const std::bad_any_cast& ex)
		{
			context->ReportError("%s", ex.what());
			return 0;
		}
	}
	static cell_t Native_InstanceReadFloat(IPluginContext* context, const cell_t* params)
	{
		SourceMod::Handle_t handle = params[1];
		SourceMod::HandleSecurity security{ context->GetIdentity(), myself->GetIdentity() };
		CSourcePawnDynamicPriority* object = nullptr;

		if (!pawnutils::ReadHandle("IDynamicWeaponPriority", context, spmanager->GetWeaponPriorityInstanceHandleType(), handle, &security, &object))
		{
			return 0;
		}

		std::size_t index = static_cast<std::size_t>(params[2]);

		if (index >= CSourcePawnDynamicPriority::MAX_SP_ARGS)
		{
			context->ReportError("Index is out of bounds! %i >= %zu", params[2], index);
			return 0;
		}

		try
		{
			float value = object->ReadData<float>(index);
			return sp_ftoc(value);
		}
		catch (const std::bad_any_cast& ex)
		{
			context->ReportError("%s", ex.what());
			return 0;
		}
	}
	static cell_t Native_InstanceReadString(IPluginContext* context, const cell_t* params)
	{
		SourceMod::Handle_t handle = params[1];
		SourceMod::HandleSecurity security{ context->GetIdentity(), myself->GetIdentity() };
		CSourcePawnDynamicPriority* object = nullptr;

		if (!pawnutils::ReadHandle("IDynamicWeaponPriority", context, spmanager->GetWeaponPriorityInstanceHandleType(), handle, &security, &object))
		{
			return 0;
		}

		std::size_t index = static_cast<std::size_t>(params[4]);

		if (index >= CSourcePawnDynamicPriority::MAX_SP_ARGS)
		{
			context->ReportError("Index is out of bounds! %i >= %zu", params[4], index);
			return 0;
		}

		try
		{
			std::string string = object->ReadData<std::string>(index);
			context->StringToLocal(params[2], static_cast<size_t>(params[3]), string.c_str());
			return 0;
		}
		catch (const std::bad_any_cast& ex)
		{
			context->ReportError("%s", ex.what());
			return 0;
		}
	}
	void setup(std::vector<sp_nativeinfo_t>& nv)
	{
		sp_nativeinfo_t list[] = {
			{"DynamicWeaponPriority.DynamicWeaponPriority", Native_CreateDynamicPriotityHandler},
			{"DynamicWeaponInstance.Priority.get", Native_InstanceGetPriority},
			{"DynamicWeaponInstance.StoreInt", Native_InstanceStoreInt},
			{"DynamicWeaponInstance.StoreFloat", Native_InstanceStoreFloat},
			{"DynamicWeaponInstance.StoreString", Native_InstanceStoreString},
			{"DynamicWeaponInstance.ReadInt", Native_InstanceReadInt},
			{"DynamicWeaponInstance.ReadFloat", Native_InstanceReadFloat},
			{"DynamicWeaponInstance.ReadString", Native_InstanceReadString},
		};

		nv.insert(nv.end(), std::begin(list), std::end(list));
	}
}