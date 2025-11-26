# SourceMod Plugin Natives

Documention for using NavBot's SourcePawn API.    
All functions are documented on the SourcePawn [include files].    

## Checking for NavBots

```

int client = ...

if (NavBotManager.IsNavBot(client))
{
    PrintToServer("Client %N is a NavBot!", client);
}
```

## Adding NavBots

```
NavBot bot = NavBot();

if (!bot.IsNull)
{
    PrintToServer("NavBot added!");
}
```

This native adds the game/mod bot implementation.    
To use a custom bot name, pass a string in the first parameter.    

## Removing NavBots

There is nothing special needed to remove NavBots. Just kick them from the game.

## Nav Mesh Status

NavBot will thrown errors if trying to create a bot without having a nav mesh loaded.    
Use `NavBotNavMesh.IsLoaded()` to check if a nav mesh is loaded for the current map.    

<!-- LINKS -->

[include files]: https://github.com/caxanga334/NavBot/tree/main/scripting/include
[Plugin Bots]: PLUGIN_BOTS.md