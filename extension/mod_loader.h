#ifndef __NAVBOT_MOD_LOADER_H_
#define __NAVBOT_MOD_LOADER_H_

#include <mods/gamemods_shared.h>
#include <ITextParsers.h>

class CBaseMod;

/**
 * @brief Class responsible for mod/game detection.
 */
class ExtModLoader : public SourceMod::ITextListener_SMC
{
public:
	ExtModLoader();
	~ExtModLoader();

	static bool ModLoaderFileExists();

	void DetectMod();

	// Allocates the class for the given detected mod.
	CBaseMod* AllocDetectedMod() const;

private:
	/* types */

	enum ModDetectionType
	{
		DETECTION_DEFAULT = 0, // default mod
		DETECTION_MOD_FOLDER,
		DETECTION_NETPROPERTY,
		DETECTION_SERVERCLASS,
		DETECTION_APPID,
	};

	/**
	 * @brief Mod entry. Stores data related to the mod
	 */
	struct ModEntry
	{
		ModEntry() :
			id(Mods::ModType::MOD_INVALID_ID), detection_type(ModDetectionType::DETECTION_DEFAULT), detection_appid(0)
		{
		}

		std::string entry; // config file entry name
		Mods::ModType id; // mod id
		std::string name; // mod name
		std::string mod_folder; // mod folder name override
		ModDetectionType detection_type; // detection type
		std::string detection_data; // string data, depends on the type
		std::string detection_sendprop_class;
		std::string detection_sendprop_property;
		std::uint64_t detection_appid;
	};

	static void CopyModEntry(const ModEntry* lhs, ModEntry* rhs)
	{
		// entry is not copied
		rhs->id = lhs->id;
		rhs->name = lhs->name;
		rhs->mod_folder = lhs->mod_folder;
		// detection data is not copied since it's unique to each mod
	}

	void ReadSMC_ParseStart() override;
	SourceMod::SMCResult ReadSMC_NewSection(const SourceMod::SMCStates* states, const char* name) override;
	SourceMod::SMCResult ReadSMC_KeyValue(const SourceMod::SMCStates* states, const char* key, const char* value) override;
	SourceMod::SMCResult ReadSMC_LeavingSection(const SourceMod::SMCStates* states) override;

	/* const */
	static constexpr int PARSER_SECTION_ROOT = 0;
	static constexpr int PARSER_SECTION_BASE = 1;
	static constexpr int PARSER_SECTION_MODENTRY = 2;
	static constexpr int PARSER_SECTION_MODDESC = 3;
	static constexpr int PARSER_SECTION_MODCONF = 4;
	static constexpr int PARSER_SECTION_MODDECT = 5;

	/* vars */
	std::vector<ModEntry> m_mods;
	const ModEntry* m_detectedMod;
	ModEntry* m_parser_current;
	int m_parser_section;

	static std::uint64_t DetectAppID();
};


#endif // !__NAVBOT_MOD_LOADER_H_
