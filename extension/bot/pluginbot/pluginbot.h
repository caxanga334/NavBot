#ifndef NAVBOT_PLUGIN_BOT_H_
#define NAVBOT_PLUGIN_BOT_H_

#include <memory>
#include <sdkports/sdk_timers.h>
#include <sdkports/sdk_ehandle.h>
#include <bot/basebot.h>
#include "pluginbot_behavior.h"
#include "pluginbot_movement.h"
#include "pluginbot.h"

struct edict_t;

class CPluginBot : public CBaseBot
{
public:
	CPluginBot(edict_t* entity);
	~CPluginBot() override;

	bool IsPluginBot() override final { return true; }

	CPluginBotMovement* GetMovementInterface() const override { return m_pluginmovement.get(); }
	CPluginBotBehavior* GetBehaviorInterface() const override { return m_pluginbehavior.get(); }

private:
	std::unique_ptr<CPluginBotMovement> m_pluginmovement;
	std::unique_ptr<CPluginBotBehavior> m_pluginbehavior;
};

#endif // !NAVBOT_PLUGIN_BOT_H_
