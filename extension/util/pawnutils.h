#ifndef __NAVBOT_PAWN_UTILS_H_
#define __NAVBOT_PAWN_UTILS_H_

#include <ISourceMod.h>
#include <amtl/am-platform.h>

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

	// Returns a pointer (Address) to sourcepawn functions.
	template <typename PointerType>
	inline cell_t ReturnPointerToPawn(IPluginContext* context, const cell_t* params, PointerType obj)
	{
		if (context->GetRuntime()->FindPubvarByName("__Int64_Address__", nullptr) == SP_ERROR_NONE)
		{
			cell_t* sp_addr = nullptr;

			if (context->LocalToPhysAddr(params[1], &sp_addr) != SP_ERROR_NONE)
			{
				context->ReportError("Could not get return adress for int64!");
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
		}
		
		return handle != BAD_HANDLE;
	}
	// Reads a handle, returns true on success
	template <typename T>
	inline bool ReadHandle(std::string_view name, IPluginContext* context, SourceMod::HandleType_t type, SourceMod::Handle_t handle, SourceMod::HandleSecurity* security, T** object)
	{
		SourceMod::HandleError error = handlesys->ReadHandle(handle, type, security, reinterpret_cast<void**>(object));
		
		if (error != SourceMod::HandleError::HandleError_None)
		{
			context->ReportError("Invalid handle %x of type \"%s\" (error %i)", name.data(), handle, static_cast<int>(error));
		}

		return error == SourceMod::HandleError::HandleError_None;
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
}


#endif // !__NAVBOT_PAWN_UTILS_H_
