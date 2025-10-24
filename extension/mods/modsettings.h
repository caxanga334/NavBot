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
		max_defend_distance = 4096.0f;
		max_sniper_distance = 8192.0f;
		rogue_chance = 8;
		rogue_max_time = 300.0f;
		rogue_min_time = 120.0f;
		movement_break_assist = true;
	}

	virtual ~CModSettings() = default;

	virtual void ParseConfigFile();
	virtual void PostParse();
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
	void SetMaxDefendDistance(float v) { max_defend_distance = v; }
	void SetMaxSniperDistance(float v) { max_sniper_distance = v; }
	void SetRogueChance(int v) { rogue_chance = v; }
	void SetRogueMaxTime(float v) { rogue_max_time = v; }
	void SetRogueMinTime(float v) { rogue_min_time = v; }
	void SetBreakAssist(bool v) { movement_break_assist = v; }

	const int GetDefendRate() const { return defendrate; }
	// Rolls a random chance to defend
	bool RollDefendChance() const;
	const int GetStuckSuicideThreshold() const { return stucksuicidethreshold; }
	const float GetUpdateRate() const { return updaterate; }
	const float GetVisionNPCUpdateRate() const { return vision_npc_update_rate; }
	const float GetInventoryUpdateRate() const { return inventory_update_rate; }
	const float GetVisionStatisticsUpdateRate() const { return vision_statistics_update; }
	const float GetCollectItemMaxDistance() const { return collect_item_max_distance; }
	const float GetMaxDefendDistance() const { return max_defend_distance; }
	const float GetMaxSniperDistance() const { return max_sniper_distance; }
	const int GetRogueBehaviorChance() const { return rogue_chance; }
	const float GetRogueBehaviorMaxTime() const { return rogue_max_time; }
	const float GetRogueBehaviorMinTime() const { return rogue_min_time; }
	const bool ShouldUseMovementBreakAssist() const { return movement_break_assist; }

protected:
	int defendrate; // percentage of bots that will do defensive tasks
	int stucksuicidethreshold; // how many stuck events before the bot kill themselves
	float updaterate; // delay in seconds between bot updates
	float vision_npc_update_rate; // delay in seconds between vision updates of non player entities
	float inventory_update_rate; // delay in seconds between updates of the weapons being carried by the bot.
	float vision_statistics_update; // delay in seconds between vision statistics updates
	float collect_item_max_distance; // maximum travel distance when collecting items (health, ammo, weapons, ...)
	float max_defend_distance; // maximum distance from the objective to search for defensive flagged waypoints
	float max_sniper_distance; // maximum distance from the objective to search for sniper flagged waypoints
	int cfg_parser_depth; // config file parser depth
	int rogue_chance; // chance for a bot to go rogue and ignore map objectives
	float rogue_max_time;
	float rogue_min_time;
	bool movement_break_assist; // (Cheat) If the bot fails to break an object within the time limit, try breaking it via inputs
};


#endif // !NAVBOT_MOD_SETTINGS_H_
