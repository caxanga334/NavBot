#ifndef __NAVBOT_CSS_BOT_H_
#define __NAVBOT_CSS_BOT_H_

#include "../basebot.h"
#include "../bot_pathcosts.h"
#include <mods/css/css_lib.h>
#include <mods/css/css_buy_profile.h>
#include "cssbot_combat.h"
#include "cssbot_inventory.h"
#include "cssbot_movement.h"
#include "cssbot_sensor.h"
#include "tasks/cssbot_main_task.h"

class CCSSBot : public CBaseBot
{
public:
	CCSSBot(edict_t* edict);
	~CCSSBot() override;

	// Gets the bot's CSS team.
	counterstrikesource::CSSTeam GetMyCSSTeam() const { return csslib::GetEntityCSSTeam(GetEntity()); }

	using CCSSBotBehavior = CDefaultBehaviorImplementation<CCSSBot, AITaskManager<CCSSBot>, CCSSBotMainTask>;

	CCSSBotBehavior* GetBehaviorInterface() const override { return m_cssbehavior.get(); }
	CCSSBotCombat* GetCombatInterface() const override { return m_csscombat.get(); }
	CCSSBotInventory* GetInventoryInterface() const override { return m_cssinventory.get(); }
	CCSSBotMovement* GetMovementInterface() const override { return m_cssmovement.get(); }
	CCSSBotSensor* GetSensorInterface() const override { return m_csssensor.get(); }
	void SetBuyProfile(const counterstrikesource::BuyProfile* profile) { m_buyprofile = profile; }
	const counterstrikesource::BuyProfile* GetBuyProfile() const { return m_buyprofile; }
	/**
	 * @brief Makes the bot send a buy command to the game server.
	 * @param item Name of the item to buy.
	 */
	void SendBuyCommand(const char* item);

private:
	std::unique_ptr<CCSSBotBehavior> m_cssbehavior;
	std::unique_ptr<CCSSBotCombat> m_csscombat;
	std::unique_ptr<CCSSBotInventory> m_cssinventory;
	std::unique_ptr<CCSSBotMovement> m_cssmovement;
	std::unique_ptr<CCSSBotSensor> m_csssensor;
	const counterstrikesource::BuyProfile* m_buyprofile;
};

class CCSSBotPathCost : public CBasicPathCost<CCSSBot>
{
public:
	CCSSBotPathCost(CCSSBot* bot, RouteType type = RouteType::DEFAULT_ROUTE) :
		CBasicPathCost<CCSSBot>(bot, type)
	{
		m_isEscortingHostages = false;
	}

	float operator()(CNavArea* toArea, CNavArea* fromArea, const CNavLadder* ladder, const NavOffMeshConnection* link, const CNavElevator* elevator, float length) const override;

	void SetEscortingHostages(bool state) { m_isEscortingHostages = state; }
	bool IsEscortingHostages() const { return m_isEscortingHostages; }

private:
	bool m_isEscortingHostages;
};

#endif // !__NAVBOT_CSS_BOT_H_
