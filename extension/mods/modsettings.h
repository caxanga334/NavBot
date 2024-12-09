#ifndef NAVBOT_MOD_SETTINGS_H_
#define NAVBOT_MOD_SETTINGS_H_

class CModSettings
{
public:
	CModSettings()
	{
		defendrate = 33;
		stucksuicidethreshold = 15; // stuck events happens every 1 second, this will be about 15 seconds
	}

	~CModSettings()
	{
	}

	void SetDefendRate(int v) { defendrate = v; }
	int GetDefendRate() const { return defendrate; }
	void SetStuckSuicideThreshold(int v) { stucksuicidethreshold = v; }
	int GetStuckSuicideThreshold() const { return stucksuicidethreshold; }

protected:

	int defendrate; // percentage of bots that will do defensive tasks
	int stucksuicidethreshold; // how many stuck events before the bot kill themselves
};


#endif // !NAVBOT_MOD_SETTINGS_H_
