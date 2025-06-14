#ifndef NAVBOT_MOD_SETTINGS_H_
#define NAVBOT_MOD_SETTINGS_H_

#include <ITextParsers.h>

class CModSettings : public SourceMod::ITextListener_SMC
{
public:
	CModSettings()
	{
		cfg_parser_depth = 0;
		defendrate = 17;
		stucksuicidethreshold = 15; // stuck events happens every 1 second, this will be about 15 seconds
		updaterate = 0.10f;
		vision_npc_update_rate = 0.250f;
		inventory_update_rate = 60.0f;
		vision_statistics_update = 0.5f;
		collect_item_max_distance = 5000.0f;
	}

	virtual ~CModSettings() = default;

	virtual void ParseConfigFile();

protected:

	void ReadSMC_ParseStart() override
	{
		cfg_parser_depth = 0;
	}

	void ReadSMC_ParseEnd(bool halted, bool failed) override {}

	SourceMod::SMCResult ReadSMC_NewSection(const SourceMod::SMCStates* states, const char* name) override;

	SourceMod::SMCResult ReadSMC_KeyValue(const SourceMod::SMCStates* states, const char* key, const char* value) override;

	SourceMod::SMCResult ReadSMC_LeavingSection(const SourceMod::SMCStates* states) override;

	SourceMod::SMCResult ReadSMC_RawLine(const SourceMod::SMCStates* states, const char* line) override { return SourceMod::SMCResult_Continue; }

public:

	void SetDefendRate(int v) { defendrate = v; }
	void SetStuckSuicideThreshold(int v) { stucksuicidethreshold = v; }
	void SetUpdateRate(float v) { updaterate = v; }
	void SetVisionNPCUpdateRate(float v) { vision_npc_update_rate = v; }
	void SetInventoryUpdateRate(float v) { inventory_update_rate = v; }
	void SetVisionStatisticsUpdateRate(float v) { vision_statistics_update = v; }
	void SetCollectItemMaxDistance(float v) { collect_item_max_distance = v; }

	const int GetDefendRate() const { return defendrate; }
	// Rolls a random chance to defend
	bool RollDefendChance() const;
	const int GetStuckSuicideThreshold() const { return stucksuicidethreshold; }
	const float GetUpdateRate() const { return updaterate; }
	const float GetVisionNPCUpdateRate() const { return vision_npc_update_rate; }
	const float GetInventoryUpdateRate() const { return inventory_update_rate; }
	const float GetVisionStatisticsUpdateRate() const { return vision_statistics_update; }
	const float GetCollectItemMaxDistance() const { return collect_item_max_distance; }

protected:
	int defendrate; // percentage of bots that will do defensive tasks
	int stucksuicidethreshold; // how many stuck events before the bot kill themselves
	float updaterate; // delay in seconds between bot updates
	float vision_npc_update_rate; // delay in seconds between vision updates of non player entities
	float inventory_update_rate; // delay in seconds between updates of the weapons being carried by the bot.
	float vision_statistics_update; // delay in seconds between vision statistics updates
	float collect_item_max_distance; // maximum travel distance when collecting items (health, ammo, weapons, ...)
	int cfg_parser_depth; // config file parser depth
};


#endif // !NAVBOT_MOD_SETTINGS_H_
