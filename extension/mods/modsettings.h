#ifndef NAVBOT_MOD_SETTINGS_H_
#define NAVBOT_MOD_SETTINGS_H_

class CModSettings
{
public:
	CModSettings()
	{
		defendrate = 33;
		stucksuicidethreshold = 15; // stuck events happens every 1 second, this will be about 15 seconds
		updaterate = 0.07f;
	}

	~CModSettings()
	{
	}

	void SetDefendRate(int v) { defendrate = v; }
	void SetStuckSuicideThreshold(int v) { stucksuicidethreshold = v; }
	void SetUpdateRate(float v) { updaterate = v; }

	int GetDefendRate() const { return defendrate; }
	int GetStuckSuicideThreshold() const { return stucksuicidethreshold; }
	const float GetUpdateRate() const { return updaterate; }

protected:

	int defendrate; // percentage of bots that will do defensive tasks
	int stucksuicidethreshold; // how many stuck events before the bot kill themselves
	float updaterate; // delay in seconds between bot updates
};


#endif // !NAVBOT_MOD_SETTINGS_H_
