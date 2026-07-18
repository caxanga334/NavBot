#ifndef __NAVBOT_PAWN_UTILS_H_
#define __NAVBOT_PAWN_UTILS_H_

#include <ISourceMod.h>
#include <IForwardSys.h>
#include <amtl/am-platform.h>
#include <sourcemod_version.h>
#include <mods/basemod.h>
#include <manager.h>

/* SourceMod Utility functions */

namespace pawnutils
{
	inline Vector PawnFloatArrayToVector(cell_t* array)
	{
		return { sp_ctof(array[0]), sp_ctof(array[1]), sp_ctof(array[2]) };
	}

	inline void VectorToPawnFloatArray(cell_t* array, const Vector& vec)
	{
		array[0] = sp_ftoc(vec.x);
		array[1] = sp_ftoc(vec.y);
		array[2] = sp_ftoc(vec.z);
	}

	// When a native returns an int64, the index of the first function parameter is shifted by one
	// This returns the correct first index to the function parameters.
	inline std::size_t GetIndexOfParam(IPluginContext* context, std::size_t index)
	{
		if (context->GetRuntime()->FindPubvarByName("__Int64_Address__", nullptr) == SP_ERROR_NONE)
		{
			return index + 1U;
		}

		return index;
	}
	inline bool IsInt64Available(SourcePawn::IPluginContext* context)
	{
		return context->GetRuntime()->FindPubvarByName("__Int64_Address__", nullptr) == SP_ERROR_NONE;
	}
	// Returns a pointer (Address) to sourcepawn functions.
	template <typename PointerType>
	inline cell_t ReturnPointerToPawn(IPluginContext* context, const cell_t* params, PointerType obj)
	{
		if (context->GetRuntime()->FindPubvarByName("__Int64_Address__", nullptr) == SP_ERROR_NONE)
		{
			cell_t* sp_addr = nullptr;

			if (context->LocalToPhysAddr(params[1], &sp_addr) != SP_ERROR_NONE)
			{
				context->ReportError("Could not get return address for int64!");
				return 0;
			}

			*reinterpret_cast<int64_t*>(sp_addr) = reinterpret_cast<uintptr_t>(obj);
			return 0;
		}

#ifdef KE_ARCH_X64
		context->ReportError("This native requires int64 support to be used on x86-64!");
		return 0;
#else
		return reinterpret_cast<uintptr_t>(obj);
#endif // KE_ARCH_X64
	}
	
	// Reads a pawn address and casts it into a pointer. Returns NULL on failures.
	// This is unsafe (there isn't any type checks).
	template <typename PointerType>
	inline PointerType* UnsafeCastPawnAddressToObject(IPluginContext* context, const cell_t* params, std::size_t index)
	{
		if (context->GetRuntime()->FindPubvarByName("__Int64_Address__", nullptr) == SP_ERROR_NONE)
		{
			cell_t* sp_addr = nullptr;

			if (context->LocalToPhysAddr(params[index], &sp_addr) != SP_ERROR_NONE)
			{
				return nullptr;
			}

			auto value = *reinterpret_cast<int64_t*>(sp_addr);
			return reinterpret_cast<PointerType*>(value);
		}

#ifdef KE_ARCH_X64
		// int64 is required for x86-64
		return nullptr;
#else
		return reinterpret_cast<PointerType*>(params[index]);
#endif // KE_ARCH_X64
	}
	// Creates a handle, returns true on success
	template <typename T>
	inline bool CreateHandle(std::string_view name, SourceMod::HandleType_t type, T* object, IPluginContext* context, SourceMod::Handle_t& handle)
	{
		SourceMod::HandleError error = SourceMod::HandleError::HandleError_None;
		handle = handlesys->CreateHandle(type, reinterpret_cast<void*>(object), context->GetIdentity(), myself->GetIdentity(), &error);

		if (handle == BAD_HANDLE)
		{
			context->ReportError("Failed to create handle of type \"%s\", error (%i).", name.data(), static_cast<int>(error));
			return false;
		}
		
		return true;
	}
	// Creates an internal (the extension is the owner) handle, returns true on success
	template <typename T>
	inline bool CreateInternalHandle(std::string_view name, SourceMod::HandleType_t type, T* object, SourceMod::Handle_t& handle)
	{
		SourceMod::HandleError error = SourceMod::HandleError::HandleError_None;
		handle = handlesys->CreateHandle(type, reinterpret_cast<void*>(object), myself->GetIdentity(), myself->GetIdentity(), &error);

		if (handle == BAD_HANDLE)
		{
			return false;
		}

		return true;
	}
	// Reads a handle, returns true on success
	template <typename T>
	inline bool ReadHandle(std::string_view name, IPluginContext* context, SourceMod::HandleType_t type, SourceMod::Handle_t handle, SourceMod::HandleSecurity* security, T** object)
	{
		SourceMod::HandleError error = handlesys->ReadHandle(handle, type, security, reinterpret_cast<void**>(object));
		
		if (error != SourceMod::HandleError::HandleError_None)
		{
			context->ReportError("Invalid handle %x of type \"%s\" (error %i)", handle, name.data(), static_cast<int>(error));
		}

		return error == SourceMod::HandleError::HandleError_None;
	}
	inline void FreeInternalHandle(SourceMod::Handle_t handle)
	{
		SourceMod::HandleSecurity sec(myself->GetIdentity(), myself->GetIdentity());
		handlesys->FreeHandle(handle, &sec);
	}
	template <typename T>
	T* GetBotOfIndex(cell_t index)
	{
		return static_cast<T*>(extmanager->GetBotByIndex(static_cast<int>(index)));
	}
	// Checks if the given index is of a valid client (player). Returns false if it's invalid. This also handles reporting errors.
	bool IsValidClientIndex(IPluginContext* context, int index);
	// Checks if the given client is in game.
	bool IsClientInGame(IPluginContext* context, int index);
	// explicit bool to cell_t conversion
	inline cell_t ReturnBool(const bool value) { return value ? static_cast<cell_t>(1) : static_cast<cell_t>(0); }
	// Returns a float value to plugins.
	inline cell_t ReturnFloat(const float value) { return sp_ftoc(value); }
	// Read a parameter into a CBaseEntity pointer.
	CBaseEntity* ReadEntity(IPluginContext* context, const cell_t* params, std::size_t index);
	// Reads a parameter into a Vector
	inline Vector ReadVector(IPluginContext* context, const cell_t* params, const std::size_t index)
	{
		cell_t* arr;
		context->LocalToPhysAddr(params[index], &arr);
		return pawnutils::PawnFloatArrayToVector(arr);
	}
	// Reads a float parameter
	inline float ReadFloat(const cell_t* params, const std::size_t index) { return sp_ctof(params[index]); }
	// Reads a boolean parameter
	inline bool ReadBool(const cell_t* params, const std::size_t index) { return params[index] != 0; }
	// Writes a vector back to SP.
	inline void WriteVector(IPluginContext* context, const cell_t* params, const std::size_t index, const Vector& vec)
	{
		cell_t* arr = nullptr;
		context->LocalToPhysAddr(params[index], &arr);
		VectorToPawnFloatArray(arr, vec);
	}
	inline bool IsNULLVector(IPluginContext* context, const cell_t* params, const std::size_t index)
	{
		cell_t* addr = nullptr;
		context->LocalToPhysAddr(params[index], &addr);
		return addr == context->GetNullRef(SourcePawn::SP_NULL_VECTOR);
	}
#if SM_BUILD_MINOR_INT >= SM_VERSION_1_13
	template <typename T>
	inline void PushObjectPtr(sp::CallArgs args, T* object)
	{
		args.PushInt64(static_cast<int64_t>(reinterpret_cast<std::uintptr_t>(object)));
	}
#endif // SM_BUILD_MINOR_INT >= SM_VERSION_1_13
	template <typename T>
	inline void PushObjectPtr(SourceMod::IForward* forward, T* object)
	{
#ifdef KE_ARCH_X64
		// object ptr not supported on x64
		forward->PushCell(0);
		return;
#else
		forward->PushCell(reinterpret_cast<std::uintptr_t>(object));
#endif // KE_ARCH_X64
	}

	inline bool ReadFunctionByID(SourcePawn::IPluginContext* context, const cell_t* params, const std::size_t index, SourcePawn::IPluginFunction** func)
	{
		*func = context->GetFunctionById(static_cast<funcid_t>(params[index]));

		if (*func == nullptr)
		{
			context->ReportError("NULL function! Param %i", params[index]);
		}

		return *func != nullptr;
	}
	// Reads a string parameter with NULL_STRING support.
	inline char* ReadStringNULL(SourcePawn::IPluginContext* context, const cell_t* params, const std::size_t index)
	{
		char* str = nullptr;
		context->LocalToStringNULL(params[index], &str);
		return str;
	}
	// Gets the mod interface pointer, returns NULL if the type doesn't match.
	template <typename T>
	inline T* GetModInterfacePointerOfType(Mods::ModType type)
	{
		CBaseMod* mod = extmanager->GetMod();

		if (mod->GetModType() != type)
		{
			return nullptr;
		}

		return static_cast<T*>(mod);
	}
	inline cell_t ReadIntByRef(SourcePawn::IPluginContext* context, const cell_t* params, const std::size_t index)
	{
		cell_t* addr;
		context->LocalToPhysAddr(params[index], &addr);
		return *addr;
	}
	inline float ReadFloatByRef(SourcePawn::IPluginContext* context, const cell_t* params, const std::size_t index)
	{
		cell_t* addr;
		context->LocalToPhysAddr(params[index], &addr);
		return sp_ctof(*addr);
	}
	template <typename T>
	inline void WriteIntByRef(SourcePawn::IPluginContext* context, const cell_t* params, const std::size_t index, T value)
	{
		static_assert(std::is_integral_v<T>);
		cell_t* addr;
		context->LocalToPhysAddr(params[index], &addr);
		*addr = static_cast<cell_t>(value);
	}
	template <typename T>
	inline void WriteFloatByRef(SourcePawn::IPluginContext* context, const cell_t* params, const std::size_t index, T value)
	{
		static_assert(std::is_floating_point_v<T>);
		cell_t* addr;
		context->LocalToPhysAddr(params[index], &addr);

		if constexpr (std::is_same_v<T, float>)
		{
			*addr = sp_ftoc(value);
			return;
		}

		*addr = sp_ftoc(static_cast<float>(value));
	}
}


#endif // !__NAVBOT_PAWN_UTILS_H_
