#ifndef __NAVBOT_NAV_MESH_ENTITY_LUMP_H_
#define __NAVBOT_NAV_MESH_ENTITY_LUMP_H_

namespace navmesh
{
	struct LumpEntity
	{
		LumpEntity()
		{
			keys.reserve(64);
		}

		std::unordered_map<std::string, std::string> keys;

		bool ClassnameIs(std::string_view name) const;

		const char* GetValue(const std::string& key) const
		{
			auto it = keys.find(key);

			if (it == keys.cend())
			{
				return nullptr;
			}

			return it->second.c_str();
		}
	};

	bool ParseMapLump(std::vector<LumpEntity>& out);
}

#endif // !__NAVBOT_NAV_MESH_ENTITY_LUMP_H_
