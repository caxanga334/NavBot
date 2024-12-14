#ifndef NAVBOT_MOD_SETTINGS_H_
#define NAVBOT_MOD_SETTINGS_H_

class CModSettings
{
public:
	CModSettings()
	{
		defendrate = 33;
		stucksuicidethreshold = 15; // stuck events happens every 1 second, this will be about 15 seconds
		updaterate = 0.10f;
		vision_npc_update_rate = 0.250f;
		inventory_update_rate = 1.0f;
	}

	~CModSettings()
	{
	}

	void SetDefendRate(int v) { defendrate = v; }
	void SetStuckSuicideThreshold(int v) { stucksuicidethreshold = v; }
	void SetUpdateRate(float v) { updaterate = v; }
	void SetVisionNPCUpdateRate(float v) { vision_npc_update_rate = v; }
	void SetInventoryUpdateRate(float v) { inventory_update_rate = v; }

	const int GetDefendRate() const { return defendrate; }
	const int GetStuckSuicideThreshold() const { return stucksuicidethreshold; }
	const float GetUpdateRate() const { return updaterate; }
	const float GetVisionNPCUpdateRate() const { return vision_npc_update_rate; }
	const float GetInventoryUpdateRate() const { return inventory_update_rate; }

protected:
	int defendrate; // percentage of bots that will do defensive tasks
	int stucksuicidethreshold; // how many stuck events before the bot kill themselves
	float updaterate; // delay in seconds between bot updates
	float vision_npc_update_rate; // delay in seconds between vision updates of non player entities
	float inventory_update_rate; // delay in seconds between updates of the weapons being carried by the bot.
};


#endif // !NAVBOT_MOD_SETTINGS_H_
