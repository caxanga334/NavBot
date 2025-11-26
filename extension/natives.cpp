#include NAVBOT_PCH_FILE
#include <extension.h>
#include "manager.h"
#include <util/helpers.h>
#include <util/pawnutils.h>
#include <bot/basebot.h>
#include <navmesh/nav_mesh.h>
#include <navmesh/nav_area.h>
#include <navmesh/nav_pathfind.h>
#include "natives.h"

class SMBuildPathCostFunctor
{
public:
	SMBuildPathCostFunctor(const float maxJump, const float maxGap, const float maxDrop)
	{
		m_maxjumpheight = maxJump;
		m_maxgapjumpdist = maxGap;
		m_maxdropheight = maxDrop;
	}

	float operator() (CNavArea* area, CNavArea* fromArea, const CNavLadder* ladder, const NavOffMeshConnection* link, const CNavElevator* elevator, float length) const
	{
		if (fromArea == nullptr)
		{
			// first area in path, no cost
			return 0.0f;
		}

		float dist = 0.0f;

		if (link != nullptr)
		{
			dist = link->GetConnectionLength();
		}
		else if (ladder != nullptr) // experimental, very few maps have 'true' ladders
		{
			dist = ladder->m_length;
		}
		else if (elevator != nullptr)
		{
			return -1.0f;
		}
		else if (length > 0.0f)
		{
			dist = length;
		}
		else
		{
			dist = (area->GetCenter() + fromArea->GetCenter()).Length();
		}

		// only check gap and height on common connections
		if (link == nullptr && elevator == nullptr && ladder == nullptr)
		{
			float deltaZ = fromArea->ComputeAdjacentConnectionHeightChange(area);

			if (deltaZ >= 18.0f)
			{
				if (deltaZ > m_maxjumpheight)
				{
					// too high to reach with regular jumps
					return -1.0f;
				}

				// jump type is resolved by the navigator

				// add jump penalty
				constexpr auto jumpPenalty = 2.0f;
				dist *= jumpPenalty;
			}
			else if (deltaZ < -m_maxdropheight)
			{
				// too far to drop
				return -1.0f;
			}

			float gap = fromArea->ComputeAdjacentConnectionGapDistance(area);

			if (gap >= m_maxgapjumpdist)
			{
				return -1.0f; // can't jump over this gap
			}
		}
		else if (link != nullptr)
		{
			switch (link->GetType())
			{
			case OffMeshConnectionType::OFFMESH_GROUND:
				[[fallthrough]];
			case OffMeshConnectionType::OFFMESH_TELEPORTER:
				break;
			default:
				return -1.0f;
			}
		}

		float cost = dist + fromArea->GetCostSoFar();

		return cost;
	}

private:
	float m_maxjumpheight;
	float m_maxgapjumpdist;
	float m_maxdropheight;
};

static std::vector<Vector> s_pawnPath;

namespace pathhelpers
{
	static void BuildTrivialPath(const Vector& start, const Vector& end)
	{
		s_pawnPath.clear();
		
		s_pawnPath.push_back(start);
		s_pawnPath.push_back(end);
	}
}

static cell_t Native_AreBotsSupported(IPluginContext* context, const cell_t* params)
{
	return extmanager->AreBotsSupported() ? 1 : 0;
}

namespace natives
{
	void setup(std::vector<sp_nativeinfo_t>& nv)
	{
		sp_nativeinfo_t list[] = {
			{"NavBotManager.IsNavBot", IsNavBot},
			{"FireNavBotSoundEvent", FireNavBotSoundEvent},
			{"NavBotManager.GetNavBotCount", GetNavBotCount},
			{"NavBotManager.AreBotsSupported", Native_AreBotsSupported},
			{"BuildPathSimple", BuildPathSimple},
			{"GetPathSegment", GetPathSegment},
			{"GetPathSegmentCount", GetPathSegmentCount},
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

		auto functor = [&entity, &origin, &type, &maxRadius](CBaseBot* bot) {
			bot->OnSound(entity, origin, type, maxRadius);
		};

		extmanager->ForEachBot(functor);

		return 0;
	}

	cell_t GetNavBotCount(IPluginContext* context, const cell_t* params)
	{
		return static_cast<cell_t>(extmanager->GetAllBots().size());
	}

	cell_t BuildPathSimple(IPluginContext* context, const cell_t* params)
	{
		if (!TheNavMesh->IsLoaded())
		{
			return 0;
		}

		cell_t* vStart;
		cell_t* vEnd;

		context->LocalToPhysAddr(params[1], &vStart);
		context->LocalToPhysAddr(params[2], &vEnd);

		Vector start = pawnutils::PawnFloatArrayToVector(vStart);
		Vector end = pawnutils::PawnFloatArrayToVector(vEnd);

		CNavArea* startArea = TheNavMesh->GetNearestNavArea(start, 512.0f);
		CNavArea* endArea = TheNavMesh->GetNearestNavArea(end, 512.0f);

		if (!startArea || !endArea)
		{
			return 0;
		}

		const float maxJumpHeight = sp_ctof(params[3]);
		const float maxGap = sp_ctof(params[4]);
		const float maxDrop = sp_ctof(params[5]);

		SMBuildPathCostFunctor cost{ maxJumpHeight, maxGap, maxDrop };

		CNavArea* closestArea = nullptr;
		bool result = NavAreaBuildPath(startArea, endArea, &end, cost, &closestArea);

		// count the number of areas in the path
		int areaCount = 0;

		for (CNavArea* area = closestArea; area != nullptr; area = area->GetParent())
		{
			areaCount++;

			if (area == startArea)
			{
				break;
			}
		}

		if (areaCount == 0)
		{
			return 0;
		}

		if (areaCount == 1)
		{
			pathhelpers::BuildTrivialPath(start, end);
			return 1;
		}

		s_pawnPath.clear();

		s_pawnPath.push_back(end);
		Vector from = end;

		for (CNavArea* area = closestArea; area != nullptr; area = area->GetParent())
		{
			CNavArea* parent = area->GetParent();

			if (!parent)
			{
				s_pawnPath.push_back(start);
				break;
			}

			NavTraverseType type = area->GetParentHow();
			Vector next;
			
			if (static_cast<int>(type) <= static_cast<int>(NavTraverseType::GO_WEST))
			{
				parent->ComputeClosestPointInPortal(area, static_cast<NavDirType>(type), from, &next);
				s_pawnPath.push_back(next);
				from = next;
			}
		}

		// Place the path start at the vector start
		std::reverse(s_pawnPath.begin(), s_pawnPath.end());

		return 1;
	}

	cell_t GetPathSegment(IPluginContext* context, const cell_t* params)
	{
		std::size_t index = static_cast<std::size_t>(params[1]);

		if (index >= s_pawnPath.size())
		{
			context->ReportError("Index %zu is out of bounds!", index);
			return 0;
		}

		cell_t* vec;
		context->LocalToPhysAddr(params[2], &vec);
		const Vector& segment = s_pawnPath[index];
		pawnutils::VectorToPawnFloatArray(vec, segment);

		return 0;
	}

	cell_t GetPathSegmentCount(IPluginContext* context, const cell_t* params)
	{
		return static_cast<cell_t>(s_pawnPath.size());
	}
}