# NavBot Global Forwards

## On Bot Add

`Action OnPreNavBotAdd()` is called when a bot is about to be added to the game. 

Returning anything higher than `Plugin_Continue` will block the bot from being added to the game.

`void OnNavBotAdded(int bot)` is called after a bot has been added to the game.

* `int bot`: Client index of the bot that was just added.

## On Create Plugin Bot

`Action OnPrePluginBotAdd(int entity)` is called when a plugin has requested to attach a NavBot Plugin Bot instance on the given entity.

Returning anything higher than `Plugin_Continue` will block the plugin bot from being created.

* `int entity`: Client index that the plugin bot will attach to.

`void OnPluginBotAdded(int entity)` is called after a plugin bot was created.

* `int entity`: Client index that the plugin bot will attach to.