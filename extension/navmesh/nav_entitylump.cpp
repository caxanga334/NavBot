#include NAVBOT_PCH_FILE
#include "nav_entitylump.h"

namespace navmesh
{
	bool LumpEntity::ClassnameIs(std::string_view name) const
	{
		auto it = keys.find("classname");

		if (it == keys.end())
		{
			return false;
		}

		return std::strcmp(it->second.c_str(), name.data()) == 0;
	}

	bool ParseMapLump(std::vector<LumpEntity>& out)
	{
		std::stringstream stream;

		{
			const char* rawstr = engine->GetMapEntitiesString();

			if (rawstr == nullptr || rawstr[0] == '\0')
			{
				return false;
			}

			stream << rawstr; // load the entity string into a stream
		}

		
		std::string line;
		std::string key;
		std::string value;
		bool section = false;
		bool string = false;
		bool escape = false;
		bool iskey = false;
		bool isvalue = false;
		int quotelevel = 0;
		LumpEntity entity;

		while (std::getline(stream, line))
		{
			// Remove carriage return and new line from the string
			line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
			line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());

			for (auto& character : line)
			{
				if (!iskey && !isvalue)
				{
					if (std::isspace(character) != 0)
					{
						continue; // skip spaces outside key/values
					}
				}

				switch (character)
				{
				case '"':
				{
					if (!section)
					{
						return false;
					}

					if (escape)
					{
						escape = false;
						continue;
					}

					// "world_maxs" "6624 3584 1392"
					// there are 4 " in a keyvalue pair

					switch (quotelevel)
					{
					case 0:
						// first quote, begin key
						iskey = true;
						quotelevel++;
						continue;
					case 1:
						// end of key
						iskey = false;
						quotelevel++;
						continue;
					case 2:
						// begin value
						isvalue = true;
						quotelevel++;
						continue;
					case 3:
					{
						// end value
						isvalue = false;
						quotelevel = 0; // reset for next line

						if (section && !key.empty() && !value.empty())
						{
							entity.keys.try_emplace(key, value);
							key.clear();
							value.clear();
						}

						continue;
					}
					default:
						break;
					}

					break;
				}
				case '{':
				{
					if (section && (!iskey && !isvalue))
					{
						return false;
					}

					if (!iskey && !isvalue)
					{
						section = true;
					}

					break;
				}
				case '}':
				{
					if (!section && (!iskey && !isvalue))
					{
						return false;
					}

					if (!iskey && !isvalue && !entity.keys.empty())
					{
						out.push_back(entity);
						entity.keys.clear();
						section = false;
					}

					break;
				}
				case '\\':
				{
					escape = true;
					break;
				}
				default:
				{
					if (iskey)
					{
						key += character;
					}

					if (isvalue)
					{
						value += character;
					}

					break;
				}
				}
			}
		}

		return true;
	}
}

#ifdef EXT_DEBUG

CON_COMMAND(sm_nav_debug_lump_parser, "Debugs the entity lump parser")
{
	std::vector<navmesh::LumpEntity> ents;
	bool result = navmesh::ParseMapLump(ents);

	META_CONPRINTF("Parse lump %s ents %zu \n", result ? "OK" : "FAILED", ents.size());
}

#endif // EXT_DEBUG
