#ifndef __NAVBOT_NAV_MESH_SETTINGS_H_
#define __NAVBOT_NAV_MESH_SETTINGS_H_

#include <ITextParsers.h>

class CNavMesh;

class CNavMapSettings : public SourceMod::ITextListener_SMC
{
public:
	CNavMapSettings();
	virtual ~CNavMapSettings();

	const std::vector<std::string>& GetAdditionalDoorClassnames() const { return m_additionaldoorclassnames; }

protected:
	static constexpr int PARSER_SECTION_ROOT = 1;
	static constexpr int PARSER_SECTION_ADDSDOORS = 2;

	int m_parser_section; // current section the parser is in


	virtual void ReadSMC_ParseStart() override;
	virtual SourceMod::SMCResult ReadSMC_NewSection(const SourceMod::SMCStates* states, const char* name) override;
	virtual SourceMod::SMCResult ReadSMC_KeyValue(const SourceMod::SMCStates* states, const char* key, const char* value) override;
	virtual SourceMod::SMCResult ReadSMC_LeavingSection(const SourceMod::SMCStates* states) override;

private:
	friend class CNavMesh;

	bool LoadFile(const std::filesystem::path& file);

	// additional classnames the nav mesh should consider as a door for auto blocker creation
	std::vector<std::string> m_additionaldoorclassnames;

	void ParseKeyValues_DoorSection(const char* key)
	{
		m_additionaldoorclassnames.emplace_back(key);
	}
};

#endif // !__NAVBOT_NAV_MESH_SETTINGS_H_
