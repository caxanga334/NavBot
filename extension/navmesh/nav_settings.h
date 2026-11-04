#ifndef __NAVBOT_NAV_MESH_SETTINGS_H_
#define __NAVBOT_NAV_MESH_SETTINGS_H_

#include <ITextParsers.h>

class CNavMesh;

class CNavMapSettings : public SourceMod::ITextListener_SMC
{
public:
	class EntityLookUpInfo
	{
	public:
		EntityLookUpInfo() :
			position(0.0f, 0.0f, 0.0f), hammerid(0)
		{
		}

		bool HasHammerID() const { return hammerid != 0; }

		CBaseEntity* FindEntity() const;

		std::string debugname;
		std::string classname;
		std::string targetname;
		Vector position;
		int hammerid;
	};

	using EntityLookUpInfoPair = std::pair<EntityLookUpInfo, EntityLookUpInfo>;

	CNavMapSettings();
	virtual ~CNavMapSettings();

	// Invoked when the round restarts
	virtual void OnRoundRestart();

	bool IsUseableObstacle(CBaseEntity* obstacle, CBaseEntity** useable) const
	{
		auto it = m_useableobstaclesmap.find(obstacle);

		if (it != m_useableobstaclesmap.cend())
		{
			CBaseEntity* ent = it->second.Get();

			if (ent == nullptr)
			{
				return false;
			}

			*useable = ent;
			return true;
		}

		return false;
	}

	const std::vector<std::string>& GetAdditionalDoorClassnames() const { return m_additionaldoorclassnames; }

protected:
	static constexpr int PARSER_SECTION_ROOT = 1;
	static constexpr int PARSER_SECTION_ADDSDOORS = 2;
	static constexpr int PARSER_SECTION_USEABLEOBSTACLES = 3;
	static constexpr int PARSER_SECTION_UO_NEWENTINFOPAIR = 4;
	static constexpr int PARSER_SECTION_UO_OBSTACLEENTINFO = 5; // Useable Obstacles: parsing the obstacle entity info
	static constexpr int PARSER_SECTION_UO_USEABLEENTINFO = 6; // Useable Obstacles: parsing the useable entity info

	int m_parser_section; // current section the parser is in
	EntityLookUpInfoPair* m_currentEntInfoPair;


	virtual void ReadSMC_ParseStart() override;
	virtual SourceMod::SMCResult ReadSMC_NewSection(const SourceMod::SMCStates* states, const char* name) override;
	virtual SourceMod::SMCResult ReadSMC_KeyValue(const SourceMod::SMCStates* states, const char* key, const char* value) override;
	virtual SourceMod::SMCResult ReadSMC_LeavingSection(const SourceMod::SMCStates* states) override;

	// Invoked after the config file is parsed
	virtual void PostParse();

private:
	friend class CNavMesh;

	bool LoadFile(const std::filesystem::path& file);

	// additional classnames the nav mesh should consider as a door for auto blocker creation
	std::vector<std::string> m_additionaldoorclassnames;
	std::vector<EntityLookUpInfoPair> m_useableobstaclesinfo;
	std::unordered_map<CBaseEntity*, CHandle<CBaseEntity>> m_useableobstaclesmap; // obstacle ptr is the key, handle points to the useable entity

	void ParseKeyValues_DoorSection(const char* key)
	{
		m_additionaldoorclassnames.emplace_back(key);
	}

	void ParseKeyValues_UseableObstacles(const char* key, const char* value, const bool isUseableEntity);

	void LinkUseableEntities();
};

#endif // !__NAVBOT_NAV_MESH_SETTINGS_H_
