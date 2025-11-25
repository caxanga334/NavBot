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

	template <typename P>
	inline cell_t PointerToPawnAddress(IPluginContext* context, P object)
	{
		if (context->GetRuntime()->FindPubvarByName("__Virtual_Address__", nullptr) == SP_ERROR_NONE)
		{
			return smutils->ToPseudoAddress(reinterpret_cast<void*>(object));
		}
#if defined(KE_ARCH_X64)
		context->ReportError("Virtual address is required for x64!");
		return 0;
#endif

		return static_cast<cell_t>(reinterpret_cast<std::uintptr_t>(object));
	}

	template <typename T>
	inline T* PawnAddressToPointer(IPluginContext* context, cell_t address)
	{
		if (context->GetRuntime()->FindPubvarByName("__Virtual_Address__", nullptr) == SP_ERROR_NONE)
		{
			return reinterpret_cast<T*>(smutils->FromPseudoAddress(static_cast<uint32_t>(address)));
		}
#if defined(KE_ARCH_X64)
		// the function that calls this will likely report an error already if it's NULL and I don't know what happens if multiple errors gets reported
		// context->ReportError("Virtual address is required for x64!");
		return nullptr;
#else /* inside #if #else so the compiler stops complaning about ( warning C4312: 'reinterpret_cast': conversion from 'cell_t' to 'T *' of greater size ) */
		if (address == 0) { return nullptr; }

		return reinterpret_cast<T*>(address);
#endif
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
	inline bool ReadHandle(std::string_view name, IPluginContext* context, SourceMod::HandleType_t type, SourceMod::Handle_t handle, SourceMod::HandleSecurity* security, T* object)
	{
		SourceMod::HandleError error = handlesys->ReadHandle(handle, type, security, reinterpret_cast<void**>(&object));
		
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
}


#endif // !__NAVBOT_PAWN_UTILS_H_
