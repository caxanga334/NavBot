#include <extension.h>
#include "manager.h"
#include "natives.h"

namespace natives
{
	//const sp_nativeinfo_t MyNatives[] =
	//{
	//	{"IsNavBot",	natives::IsNavBot},
	//	{nullptr,			nullptr},
	//};

	void setup(std::vector<sp_nativeinfo_t>& nv)
	{
		sp_nativeinfo_t list[] = {
			{"IsNavBot", IsNavBot},
			{"AddNavBot", AddNavBot},
			{"IsNavMeshLoaded", IsNavMeshLoaded},
		};

		nv.insert(nv.end(), std::begin(list), std::end(list));
	}

	cell_t IsNavBot(IPluginContext* context, const cell_t* params)
	{
		int client = params[1];

		if (client < 0 || client > gpGlobals->maxClients)
		{
			return context->ThrowNativeError("Invalid client index %i!", client);
		}

		bool isbot = extmanager->IsNavBot(client);

		return isbot ? 1 : 0;
	}

	cell_t AddNavBot(IPluginContext* context, const cell_t* params)
	{
		if (!TheNavMesh->IsLoaded())
		{
			context->ReportError("Cannot add bot. Navigation Mesh is not loaded!");
			return 0;
		}

		cell_t* client = nullptr;
		edict_t* edict = nullptr;

		context->LocalToPhysAddr(params[1], &client);

		char* szName = nullptr;

		context->LocalToStringNULL(params[2], &szName);

		if (szName != nullptr)
		{
			std::string name(szName);

			extmanager->AddBot(&name, &edict);
		}
		else
		{
			extmanager->AddBot(nullptr, &edict);
		}
		
		if (edict == nullptr)
		{
			*client = 0;
			return 0;
		}

		*client = static_cast<cell_t>(gamehelpers->IndexOfEdict(edict));

		return 1;
	}

	cell_t IsNavMeshLoaded(IPluginContext* context, const cell_t* params)
	{
		return TheNavMesh->IsLoaded() ? 1 : 0;
	}
}