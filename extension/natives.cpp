#include <extension.h>
#include "manager.h"
#include <bot/basebot.h>
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
			{"FireNavBotSoundEvent", FireNavBotSoundEvent},
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

	cell_t FireNavBotSoundEvent(IPluginContext* context, const cell_t* params)
	{
		CBaseEntity* entity = gamehelpers->ReferenceToEntity(params[1]);

		if (!entity)
		{
			context->ReportError("%i is not a valid entity index/reference!", params[1]);
			return 0;
		}

		Vector origin = UtilHelpers::getWorldSpaceCenter(entity);

		cell_t* addr = nullptr;
		context->LocalToPhysAddr(params[2], &addr);

		if (addr != context->GetNullRef(SourcePawn::SP_NULL_VECTOR))
		{
			origin.x = sp_ctof(addr[0]);
			origin.y = sp_ctof(addr[1]);
			origin.z = sp_ctof(addr[2]);
		}

		IEventListener::SoundType type = static_cast<IEventListener::SoundType>(params[3]);

		if (type < IEventListener::SoundType::SOUND_GENERIC || type >= IEventListener::SoundType::MAX_SOUND_TYPES)
		{
			context->ReportError("%i is not a valid sound type!", static_cast<int>(type));
			return 0;
		}

		float maxRadius = sp_ctof(params[4]);

		extmanager->ForEachBot([&entity, &origin, &type, &maxRadius](CBaseBot* bot) {
			bot->OnSound(entity, origin, type, maxRadius);
		});

		return 0;
	}
}