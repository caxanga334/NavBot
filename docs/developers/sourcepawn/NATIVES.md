# SourceMod Plugin Natives

Documention for using NavBot's SourcePawn API.

All functions are documented on the SourcePawn [include files].

## Checking for NavBots

Use `bool IsNavBot(int client)`.

It will return true if there is a NavBot instance attached to the given client index.

This returns true for all types of NavBots.

## Adding NavBots

To add NavBots to the game, plugins can call the `bool AddNavBot(int& client, const char[] botName = NULL_STRING)` function.

This will automatically create a new fake client and attach a NavBot instance to it.

This function returns true when the bot was successfully created. Returns false on failure.

* `int& client`: Client index of the bot that was just added (passed by reference).
* `const char[] botName`: Optional, if set to `NULL_STRING`, NavBot will automatically choose a random name from it's list, otherwise it will use the name specified here.

This native adds the game/mod bot implementation. To create a plugin bot, see the [Plugin Bots] documentation.

## Removing NavBots

There is nothing special needed to remove NavBots. Just kick them from the game.

## Nav Mesh Status

NavBot will thrown errors if trying to create a bot without having a nav mesh loaded.

Use `bool IsNavMeshLoaded()` to check if a nav mesh is loaded for the current map.

<!-- LINKS -->

[include files]: https://github.com/caxanga334/NavBot/tree/main/scripting/include
[Plugin Bots]: PLUGIN_BOTS.md