#include <extension.h>
#include <utlbuffer.h>
#include <filesystem.h>
#include <util/helpers.h>
#include <util/entprops.h>
#include <limits>
#include "tfnavmesh.h"
#include "tfnavarea.h"

#undef max
#undef min

void CTFNavArea::Save(CUtlBuffer& fileBuffer, unsigned int version, unsigned int portversion) const
{
	CNavArea::Save(fileBuffer, version, portversion); // Save base first

	fileBuffer.PutInt(m_tfattributes);
	fileBuffer.PutInt(m_tfpathattributes);
}

NavErrorType CTFNavArea::Load(CUtlBuffer& fileBuffer, unsigned int version, unsigned int portversion, unsigned int subVersion)
{
	auto base = CNavArea::Load(fileBuffer, version, portversion, subVersion); // Load base first

	if (base != NAV_OK)
	{
		return base;
	}

	m_tfattributes = fileBuffer.GetInt();
	m_tfpathattributes = fileBuffer.GetInt();

	if (!fileBuffer.IsValid())
	{
		return NAV_CORRUPT_DATA;
	}

	return NAV_OK;
}

void CTFNavArea::UpdateDynamicSpawnRoom()
{
	int spawnroom = 0;
	const Vector& center = GetCenter();
	int nearest_team = 0; // store the team of the nearest func_respawnroom entity
	float smallest_dist = std::numeric_limits<float>::max();

	while ((spawnroom = UtilHelpers::FindEntityByClassname(spawnroom, "func_respawnroom")) != INVALID_EHANDLE_INDEX)
	{
		edict_t* edict = nullptr;
		
		if (!UtilHelpers::IndexToAThings(spawnroom, nullptr, &edict))
		{
			continue;
		}

		auto spawnroompos = UtilHelpers::getWorldSpaceCenter(edict);

		float current = center.DistTo(spawnroompos);

		if (current < smallest_dist)
		{
			smallest_dist = current;

			int lookup = 0;
			if (entprops->GetEntProp(spawnroom, Prop_Data, "m_iTeamNum", lookup))
			{
				nearest_team = lookup;
			}
		}
	}

	m_spawnroomteam = nearest_team;
}
