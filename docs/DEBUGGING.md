# Debugging NavBot

Navbot features a debugging system similar to Valve's NextBots.    
The debug system is only available on a Listen server becasue it relies on the engine's debug overlay interface.    

## Enabling Debug

To enable debug, use the `sm_navbot_debug <OPTION1> ... <OPTIONSN>` console command.    
The following options are available:    

* STOPALL : Disables all debugging options.
* SENSOR : Debugs the bot's sensor interface.
* TASKS : Debugs the bot's behavior interface.
* LOOK : Debugs the bot's look/aim at commands.
* PATH : Debugs the bot's path/navigator.
* EVENTS : Debugs the event listener interface.
* MOVEMENT : Debugs the bot's movement interface.
* ERRORS : Show debug error messages.
* SQUADS : Debugs the bot's squad interface.
* MISC : Debugs miscellaneous bot functions.
* COMBAT : Debugs the bot's combat functions.

### Filtering

Having too many debug overlays being rendered at once will crash the game, if you need to debug with multiple bots present, you can specify which bot you want to debug with the `sm_navbot_debug_filter` ConVar.    
Set the ConVar value the entity index of the bot you want to debug.    